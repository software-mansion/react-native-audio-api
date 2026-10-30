// The phone's sound: notes play from SampleBank's baked voices (live
// Voices.ts graphs until a sample is ready), and every note feeds one output
// chain the phone's console shapes live:
//   master gain → lowpass filter → soft clip → mute → speaker
import type {
  AudioBuffer,
  AudioContext,
  AudioParam,
  BiquadFilterNode,
  GainNode,
  WaveShaperNode,
} from 'react-native-audio-api';

import { DRUMS } from '../../../orchestra-server/drumKit';
import InstrumentBank, {
  type InstrumentRequest,
  type RecordedTake,
} from './InstrumentBank';
import SampleBank, {
  PAD_BAKED_ATTACK_SEC,
  steeperDecayTimeConstant,
  type Sample,
  type SampleRequest,
} from './SampleBank';
import {
  clamp,
  createNoise,
  leadRelease,
  midiToFrequency,
  padAttack,
  padRelease,
  playVoice,
  resolveTimbre,
  SILENT,
  type VoiceTimbre,
} from './Voices';
import type { Timbre } from './protocol';

const MASTER_GAIN = 0.6;
/** Time constant of the mute fade: ~95% gone within 30 ms, without a click. */
const MUTE_FADE_SECONDS = 0.01;
/** Console moves glide over ~60 ms so a finger dragging the pad never zips. */
const CONSOLE_GLIDE_SECONDS = 0.02;
const FILTER_MIN_HZ = 150;
const FILTER_MAX_HZ = 18000;
const FILTER_MIN_Q = 0.7;
const FILTER_MAX_Q = 14;
/** How far past full scale the soft clip still bends instead of flattening. */
const SOFT_CLIP_HEADROOM = 3;
const SOFT_CLIP_CURVE_LENGTH = 2049;
const STOP_MARGIN_SEC = 0.02;
/** Recorded notes are mastered hotter than the synth voices. */
const RECORDED_LEVEL = 0.8;
/** A released recorded note fades out over ~5 of these. */
const RELEASE_TIME_CONSTANT_SEC = 0.06;

// tanh over [-headroom, headroom], normalised so that a small signal passes
// at unity gain once the master gain divides by the same headroom.
function softClipCurve(): Float32Array {
  const curve = new Float32Array(SOFT_CLIP_CURVE_LENGTH);
  for (let i = 0; i < SOFT_CLIP_CURVE_LENGTH; i += 1) {
    const x = (i / (SOFT_CLIP_CURVE_LENGTH - 1)) * 2 - 1;
    curve[i] = Math.tanh(x * SOFT_CLIP_HEADROOM);
  }
  return curve;
}

export default class Synth {
  private readonly context: AudioContext;
  private readonly master: GainNode;
  private readonly filter: BiquadFilterNode;
  private readonly softClip: WaveShaperNode;
  private readonly output: GainNode;
  private readonly noise: AudioBuffer;
  private readonly samples: SampleBank;
  private readonly instruments: InstrumentBank;

  /** `sampleBaseUrl`: the orchestra server over http, for recorded instruments. */
  constructor(context: AudioContext, sampleBaseUrl: string | null) {
    this.context = context;

    this.master = context.createGain();
    this.master.gain.value = MASTER_GAIN / SOFT_CLIP_HEADROOM;
    this.filter = context.createBiquadFilter();
    this.filter.type = 'lowpass';
    this.filter.frequency.value = FILTER_MAX_HZ;
    this.filter.Q.value = FILTER_MIN_Q;
    this.softClip = context.createWaveShaper();
    this.softClip.curve = softClipCurve();
    this.output = context.createGain();
    this.master.connect(this.filter);
    this.filter.connect(this.softClip);
    this.softClip.connect(this.output);
    this.output.connect(context.destination);

    this.noise = createNoise(context);
    this.samples = new SampleBank(context);
    this.instruments = new InstrumentBank(context, sampleBaseUrl);
  }

  /** Fades the whole output, including notes already scheduled. */
  setMuted(muted: boolean): void {
    this.glide(this.output.gain, muted ? 0 : 1, MUTE_FADE_SECONDS);
  }

  /** Both 0..1: cutoff sweeps 150 Hz – 18 kHz on a musical (log) scale. */
  setFilter(cutoff: number, resonance: number): void {
    const hz =
      FILTER_MIN_HZ *
      Math.pow(FILTER_MAX_HZ / FILTER_MIN_HZ, clamp(cutoff, 0, 1));
    const q =
      FILTER_MIN_Q +
      (FILTER_MAX_Q - FILTER_MIN_Q) * clamp(resonance, 0, 1) ** 2;
    this.glide(this.filter.frequency, hz, CONSOLE_GLIDE_SECONDS);
    this.glide(this.filter.Q, q, CONSOLE_GLIDE_SECONDS);
  }

  private glide(param: AudioParam, target: number, timeConstant: number): void {
    const now = this.context.currentTime;
    param.cancelScheduledValues(now);
    param.setValueAtTime(param.value, now);
    param.setTargetAtTime(target, now, timeConstant);
  }

  /** Starts rendering these voices now, so their first notes play baked. */
  prepare(requests: readonly SampleRequest[]): void {
    this.samples.prepare(requests);
  }

  /** Starts loading these recorded notes now, so they are ready when played. */
  prepareInstruments(requests: readonly InstrumentRequest[]): void {
    this.instruments.prepare(requests);
  }

  play(
    time: number,
    midi: number,
    durationSec: number,
    velocity: number,
    timbre: Timbre,
    instrument?: string
  ): void {
    // A zero peak would make the exponential ramps that follow it undefined.
    const level = clamp(velocity, 0.01, 1);
    if (instrument) {
      const recorded = this.instruments.get(instrument, midi);
      if (recorded) {
        // A drum hit rings out however short the score writes it.
        const sounding =
          instrument === DRUMS ? recorded.buffer.duration : durationSec;
        this.playRecorded(recorded, time, sounding, level);
        return;
      }
      // Still loading (or missing on the server): the synth fills in.
      this.instruments.prepare([{ instrument, midi }]);
    }

    const voice = resolveTimbre(timbre, midi);
    const sample = this.samples.get(voice, midi);
    if (sample) {
      this.playSample(sample, voice, time, durationSec, level);
      return;
    }
    playVoice(this.context, this.master, this.noise, voice, {
      time,
      frequency: midiToFrequency(midi),
      durationSec,
      velocity: level,
    });
    this.samples.prepare([{ timbre: voice, midi }]);
  }

  // A recorded note keeps its own attack and decay; the gain only releases it
  // when the score's note ends, like letting go of a key. A sound that holds
  // loops its last stretch for as long as the note outlasts the recording.
  private playRecorded(
    { buffer, loop }: RecordedTake,
    time: number,
    durationSec: number,
    level: number
  ): void {
    const source = this.context.createBufferSource();
    source.buffer = buffer;
    if (loop) {
      source.loop = true;
      source.loopStart = loop.startSec;
      source.loopEnd = loop.endSec;
    }
    const gain = this.context.createGain();
    source.connect(gain);
    gain.connect(this.master);

    const peak = RECORDED_LEVEL * level;
    const release = time + durationSec;
    gain.gain.value = peak;
    gain.gain.setValueAtTime(peak, release);
    gain.gain.setTargetAtTime(0, release, RELEASE_TIME_CONSTANT_SEC);
    source.start(time);
    const released = release + RELEASE_TIME_CONSTANT_SEC * 6;
    source.stop(
      (loop ? released : Math.min(time + buffer.duration, released)) +
        STOP_MARGIN_SEC
    );
  }

  private playSample(
    { buffer, shape }: Sample,
    voice: VoiceTimbre,
    time: number,
    durationSec: number,
    level: number
  ): void {
    const source = this.context.createBufferSource();
    source.buffer = buffer;
    const gain = this.context.createGain();
    source.connect(gain);
    gain.connect(this.master);

    if (shape.kind === 'decaying') {
      const length = shape.lengthOf(durationSec);
      gain.gain.value = level;
      const timeConstant = steeperDecayTimeConstant(shape, length);
      if (timeConstant !== null) {
        const decayFrom = time + shape.decayFromSec;
        gain.gain.setValueAtTime(level, decayFrom);
        gain.gain.setTargetAtTime(0, decayFrom, timeConstant);
      }
      source.start(time);
      source.stop(time + Math.min(length, buffer.duration) + STOP_MARGIN_SEC);
      return;
    }

    source.loop = true;
    source.loopStart = shape.loopStartSec;
    source.loopEnd = shape.loopEndSec;

    const { holdUntil, end } =
      voice === 'pad' ? padRelease(durationSec) : leadRelease(durationSec);
    // The pad is baked with its slowest attack: a quicker one starts reading
    // later into it and fades in over what is left.
    const attack = voice === 'pad' ? padAttack(durationSec) : 0;
    const offset = voice === 'pad' ? PAD_BAKED_ATTACK_SEC - attack : 0;

    gain.gain.value = attack > 0 ? 0 : level;
    if (attack > 0) {
      gain.gain.setValueAtTime(0, time);
      gain.gain.linearRampToValueAtTime(level, time + attack);
    }
    gain.gain.setValueAtTime(level, time + holdUntil);
    gain.gain.exponentialRampToValueAtTime(SILENT, time + end);
    source.start(time, offset);
    source.stop(time + end + STOP_MARGIN_SEC);
  }

  dispose(): void {
    this.samples.dispose();
    this.instruments.dispose();
    this.master.disconnect();
    this.filter.disconnect();
    this.softClip.disconnect();
    this.output.disconnect();
  }
}

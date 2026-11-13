// Per-note voices tuned for phone speakers: bright spectra, nothing that
// depends on bass, every envelope scheduled at `time` up front. Each note
// builds its own small graph, released by its sources' `stop`. They run on any
// context: live on the phone's, or once per pitch on an offline context to
// bake the samples SampleBank plays.
import type {
  AudioBuffer,
  AudioNode,
  AudioParam,
  BaseAudioContext,
  OscillatorNode,
} from 'react-native-audio-api';

import { DRUM_PIECES, drumPieceFor } from '../../../orchestra-server/drumKit';
import type { Timbre } from './protocol';

/** A kit note resolves to one of these before it reaches a voice. */
export type VoiceTimbre = Exclude<Timbre, 'kit'>;

export interface VoiceNote {
  time: number;
  frequency: number;
  durationSec: number;
  /** 0.01..1 */
  velocity: number;
}

const NOISE_SECONDS = 1;
/** Exponential ramps cannot reach zero. */
export const SILENT = 0.0001;
const STOP_MARGIN_SEC = 0.02;

export function midiToFrequency(midi: number): number {
  return 440 * Math.pow(2, (midi - 69) / 12);
}

export function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

export function resolveTimbre(timbre: Timbre, midi: number): VoiceTimbre {
  if (timbre !== 'kit') {
    return timbre;
  }
  // The synth stand-in for the recorded kit: cymbals hiss, skins clap.
  const piece = drumPieceFor(midi);
  return piece && DRUM_PIECES[piece].metal ? 'hat' : 'clap';
}

export function createNoise(context: BaseAudioContext): AudioBuffer {
  const length = Math.round(context.sampleRate * NOISE_SECONDS);
  const noise = context.createBuffer(1, length, context.sampleRate);
  const samples = new Float32Array(length);
  for (let i = 0; i < length; i += 1) {
    samples[i] = Math.random() * 2 - 1;
  }
  noise.copyToChannel(samples, 0);
  return noise;
}

export function playVoice(
  context: BaseAudioContext,
  destination: AudioNode,
  noise: AudioBuffer,
  timbre: VoiceTimbre,
  note: VoiceNote
): void {
  const voices = new VoiceBuilder(context, destination, noise);
  switch (timbre) {
    case 'bell':
      return voices.bell(note);
    case 'pluck':
      return voices.pluck(note);
    case 'pad':
      return voices.pad(note);
    case 'lead':
      return voices.lead(note);
    case 'hat':
      return voices.hat(note);
    case 'clap':
      return voices.clap(note);
    case 'click':
      return voices.click(note);
  }
}

class VoiceBuilder {
  constructor(
    private readonly context: BaseAudioContext,
    private readonly destination: AudioNode,
    private readonly noise: AudioBuffer
  ) {}

  // FM: the modulation index decays faster than the amplitude, so the strike
  // is metallic and the tail settles into a pure tone.
  bell({ time, frequency, durationSec, velocity }: VoiceNote): void {
    const end = time + bellLength(durationSec);
    const amplitude = this.envelopeOutput();
    percussiveEnvelope(amplitude.gain, time, 0.8 * velocity, 0.002, end);

    const carrier = this.context.createOscillator();
    carrier.frequency.value = frequency;
    const modulator = this.context.createOscillator();
    modulator.frequency.value = frequency * 3.5;

    const modulationDepth = this.context.createGain();
    modulationDepth.gain.setValueAtTime(frequency * 2.5 * velocity, time);
    modulationDepth.gain.exponentialRampToValueAtTime(
      frequency * 0.05,
      time + Math.min(0.5, end - time)
    );

    modulator.connect(modulationDepth);
    modulationDepth.connect(carrier.frequency);
    carrier.connect(amplitude);

    this.startAll([carrier, modulator], time, end);
  }

  pluck({ time, frequency, durationSec, velocity }: VoiceNote): void {
    const end = time + pluckLength(durationSec);
    const amplitude = this.envelopeOutput();
    const peak = 0.45 * velocity;
    amplitude.gain.setValueAtTime(0, time);
    amplitude.gain.linearRampToValueAtTime(peak, time + 0.002);
    amplitude.gain.exponentialRampToValueAtTime(peak * 0.35, time + 0.1);
    amplitude.gain.exponentialRampToValueAtTime(SILENT, end);

    const filter = this.context.createBiquadFilter();
    filter.type = 'lowpass';
    filter.Q.value = 3;
    filter.frequency.setValueAtTime(
      Math.min(frequency * (6 + 10 * velocity), 14000),
      time
    );
    filter.frequency.exponentialRampToValueAtTime(
      Math.max(frequency * 1.5, 500),
      time + 0.18
    );
    filter.connect(amplitude);

    const saw = this.context.createOscillator();
    saw.type = 'sawtooth';
    saw.frequency.value = frequency;
    saw.connect(filter);

    const square = this.context.createOscillator();
    square.type = 'square';
    square.frequency.value = frequency;
    square.detune.value = 6;
    const squareLevel = this.context.createGain();
    squareLevel.gain.value = 0.4;
    square.connect(squareLevel);
    squareLevel.connect(filter);

    this.startAll([saw, square], time, end);
  }

  pad({ time, frequency, durationSec, velocity }: VoiceNote): void {
    const attack = padAttack(durationSec);
    const { holdUntil, end } = padRelease(durationSec);
    const peak = 0.12 * velocity;

    const amplitude = this.envelopeOutput();
    amplitude.gain.setValueAtTime(0, time);
    amplitude.gain.linearRampToValueAtTime(peak, time + attack);
    amplitude.gain.setValueAtTime(peak, time + holdUntil);
    amplitude.gain.exponentialRampToValueAtTime(SILENT, time + end);

    // The filter opens with the attack so the chord swells, not just fades in.
    const filter = this.context.createBiquadFilter();
    filter.type = 'lowpass';
    filter.Q.value = 0.8;
    filter.frequency.setValueAtTime(frequency * 2, time);
    filter.frequency.linearRampToValueAtTime(
      Math.min(frequency * 5, 6000),
      time + attack
    );
    filter.connect(amplitude);

    const voices = [-9, 0, 8].map((detuneCents) => {
      const saw = this.context.createOscillator();
      saw.type = 'sawtooth';
      saw.frequency.value = frequency;
      saw.detune.value = detuneCents;
      saw.connect(filter);
      return saw;
    });

    this.startAll(voices, time, time + end);
  }

  lead({ time, frequency, durationSec, velocity }: VoiceNote): void {
    const attack = 0.04;
    const decayEnd = time + 0.2;
    const { holdUntil, end } = leadRelease(durationSec);
    const peak = 0.3 * velocity;

    const amplitude = this.envelopeOutput();
    amplitude.gain.setValueAtTime(0, time);
    amplitude.gain.linearRampToValueAtTime(peak, time + attack);
    amplitude.gain.linearRampToValueAtTime(peak * 0.75, decayEnd);
    amplitude.gain.setValueAtTime(peak * 0.75, time + holdUntil);
    amplitude.gain.exponentialRampToValueAtTime(SILENT, time + end);

    const filter = this.context.createBiquadFilter();
    filter.type = 'lowpass';
    filter.Q.value = 1;
    filter.frequency.value = Math.min(frequency * 6, 9000);
    filter.connect(amplitude);

    const saw = this.context.createOscillator();
    saw.type = 'sawtooth';
    saw.frequency.value = frequency;
    saw.detune.value = 4;
    saw.connect(filter);

    const triangle = this.context.createOscillator();
    triangle.type = 'triangle';
    triangle.frequency.value = frequency;
    triangle.connect(filter);

    // Vibrato fades in only once the note has settled, like a singer's.
    const vibrato = this.context.createOscillator();
    vibrato.frequency.value = 5.5;
    const vibratoCents = this.context.createGain();
    const vibratoOnset = time + 0.25;
    vibratoCents.gain.setValueAtTime(0, time);
    vibratoCents.gain.setValueAtTime(0, vibratoOnset);
    vibratoCents.gain.linearRampToValueAtTime(18, vibratoOnset + 0.4);
    vibrato.connect(vibratoCents);
    vibratoCents.connect(saw.detune);
    vibratoCents.connect(triangle.detune);

    this.startAll([saw, triangle, vibrato], time, time + end);
  }

  hat({ time, durationSec, velocity }: VoiceNote): void {
    const end = time + hatLength(durationSec);
    const amplitude = this.envelopeOutput();
    percussiveEnvelope(amplitude.gain, time, 0.45 * velocity, 0.001, end);

    const filter = this.context.createBiquadFilter();
    filter.type = 'highpass';
    filter.frequency.value = 7000;
    filter.Q.value = 0.9;
    filter.connect(amplitude);

    this.playNoise(filter, time, end);
  }

  // A hand clap is a few rapid bursts followed by a short diffuse tail.
  clap({ time, frequency, durationSec, velocity }: VoiceNote): void {
    const burstSpacing = 0.011;
    const burstCount = 3;
    const tailStart = time + burstSpacing * burstCount;
    const end = time + clapLength(durationSec);
    const peak = 0.7 * velocity;

    const amplitude = this.envelopeOutput();
    amplitude.gain.setValueAtTime(0, time);
    for (let burst = 0; burst < burstCount; burst += 1) {
      const burstStart = time + burst * burstSpacing;
      amplitude.gain.setValueAtTime(peak, burstStart);
      amplitude.gain.exponentialRampToValueAtTime(
        peak * 0.15,
        burstStart + burstSpacing * 0.8
      );
    }
    amplitude.gain.setValueAtTime(peak, tailStart);
    amplitude.gain.exponentialRampToValueAtTime(SILENT, end);

    const filter = this.context.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.value = clamp(frequency * 2, 800, 3000);
    filter.Q.value = 1.2;
    filter.connect(amplitude);

    this.playNoise(filter, time, end);
  }

  // The sync-test sound: as short and sharp as possible.
  click({ time, frequency, durationSec, velocity }: VoiceNote): void {
    const end = time + clickLength(durationSec);
    const amplitude = this.envelopeOutput();
    percussiveEnvelope(amplitude.gain, time, velocity, 0.001, end);

    const square = this.context.createOscillator();
    square.type = 'square';
    square.frequency.value = frequency;
    const squareLevel = this.context.createGain();
    squareLevel.gain.value = 0.8;
    square.connect(squareLevel);
    squareLevel.connect(amplitude);

    const overtone = this.context.createOscillator();
    overtone.frequency.value = frequency * 3;
    const overtoneLevel = this.context.createGain();
    overtoneLevel.gain.value = 0.2;
    overtone.connect(overtoneLevel);
    overtoneLevel.connect(amplitude);

    this.startAll([square, overtone], time, end);
  }

  private envelopeOutput() {
    const gain = this.context.createGain();
    gain.gain.value = 0;
    gain.connect(this.destination);
    return gain;
  }

  private playNoise(destination: AudioNode, time: number, end: number): void {
    const source = this.context.createBufferSource();
    source.buffer = this.noise;
    source.connect(destination);
    // A random read position keeps repeated hits from sounding identical.
    const longestHit = 0.5;
    source.start(time, Math.random() * (NOISE_SECONDS - longestHit));
    source.stop(end + STOP_MARGIN_SEC);
  }

  private startAll(sources: OscillatorNode[], time: number, end: number): void {
    for (const source of sources) {
      source.start(time);
      source.stop(end + STOP_MARGIN_SEC);
    }
  }
}

function percussiveEnvelope(
  gain: AudioParam,
  time: number,
  peak: number,
  attack: number,
  end: number
): void {
  gain.setValueAtTime(0, time);
  gain.linearRampToValueAtTime(peak, time + attack);
  gain.exponentialRampToValueAtTime(SILENT, end);
}

// How long each voice sounds for a note of `durationSec`, relative to its
// start. Shared with SampleBank, which bakes the longest version of each and
// shortens it on playback.

export const bellLength = (durationSec: number) =>
  clamp(durationSec * 1.5, 0.6, 3);
export const pluckLength = (durationSec: number) =>
  clamp(durationSec * 1.2, 0.2, 1.4);
export const hatLength = (durationSec: number) => clamp(durationSec, 0.03, 0.4);
export const clapLength = (durationSec: number) =>
  0.033 + clamp(durationSec, 0.08, 0.3);
export const clickLength = (durationSec: number) =>
  0.001 + Math.max(0.03, durationSec);

export const padAttack = (durationSec: number) =>
  clamp(durationSec * 0.3, 0.05, 0.5);

export function padRelease(durationSec: number) {
  return {
    holdUntil: Math.max(padAttack(durationSec) + 0.01, durationSec * 0.8),
    end: durationSec + 0.3,
  };
}

export function leadRelease(durationSec: number) {
  const holdUntil = Math.max(0.21, durationSec);
  return { holdUntil, end: holdUntil + 0.15 };
}

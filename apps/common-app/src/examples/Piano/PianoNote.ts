import type {
  AudioBufferSourceNode,
  AudioContext,
  GainNode,
} from 'react-native-audio-api';

import { SampleMap, sampleFor } from './samples';

/**
 * Minimum time a note rings after its key goes down, like a light sustain
 * pedal.
 */
const minimumRingTime = 5.0;
const releaseTime = 0.08;

/**
 * Lowpass cutoff of the softest note, in Hz. A piano struck softly sounds
 * darker as well as quieter; the cutoff opens by `brightnessOctaves` at full
 * velocity.
 */
const softestCutoff = 700;
const brightnessOctaves = 5;

/** Loudness grows with the square of velocity, about 28 dB over 0.2–1. */
function velocityToGain(velocity: number) {
  return velocity * velocity;
}

function velocityToCutoff(velocity: number) {
  return softestCutoff * 2 ** (brightnessOctaves * velocity);
}

export interface Voice {
  source: AudioBufferSourceNode;
  gain: GainNode;
  startedAt: number;
  /** Audio-clock time the source stops, once the voice has been released. */
  stopsAt?: number;
}

/** Starts the nearest sample, retuned to `midi`, at `when` on the audio clock. */
function startVoice(
  audioContext: AudioContext,
  samples: SampleMap,
  midi: number,
  when: number,
  velocity = 1
): Voice | null {
  const sample = sampleFor(samples, midi);

  if (!sample) {
    return null;
  }

  const source = audioContext.createBufferSource();
  source.buffer = sample.buffer;
  source.playbackRate.value = sample.playbackRate;

  const tone = audioContext.createBiquadFilter();
  tone.type = 'lowpass';
  tone.Q.value = 0;
  tone.frequency.value = Math.min(
    audioContext.sampleRate / 2 - 100,
    velocityToCutoff(velocity)
  );

  const gain = audioContext.createGain();
  gain.gain.setValueAtTime(velocityToGain(velocity), when);

  source.connect(tone);
  tone.connect(gain);
  gain.connect(audioContext.destination);
  source.start(when);

  return { source, gain, startedAt: when };
}

/**
 * Releases a voice at `keyUpAt`, but never before it has rung for
 * `minimumRingTime`.
 */
function releaseVoice(voice: Voice, keyUpAt: number) {
  const releaseAt = Math.max(keyUpAt, voice.startedAt + minimumRingTime);

  voice.stopsAt = releaseAt + releaseTime + 0.02;
  voice.gain.gain.exponentialRampToValueAtTime(0.0001, releaseAt + releaseTime);
  voice.gain.gain.setValueAtTime(0, releaseAt + releaseTime + 0.01);
  voice.source.stop(voice.stopsAt);
}

/** Schedules a whole note ahead of time: key down at `start`, key up at `end`. */
export function scheduleNote(
  audioContext: AudioContext,
  samples: SampleMap,
  midi: number,
  start: number,
  end: number,
  velocity = 1
) {
  const voice = startVoice(audioContext, samples, midi, start, velocity);

  if (voice) {
    releaseVoice(voice, end);
  }

  return voice;
}

/** A key played live: it sounds on `start` and is released on `stop`. */
class PianoNote {
  private voice: Voice | null = null;

  constructor(
    private readonly audioContext: AudioContext,
    private readonly midi: number
  ) {}

  start(samples: SampleMap) {
    this.voice = startVoice(
      this.audioContext,
      samples,
      this.midi,
      this.audioContext.currentTime
    );
  }

  stop() {
    if (!this.voice) {
      return;
    }

    releaseVoice(this.voice, this.audioContext.currentTime);
    this.voice = null;
  }
}

export default PianoNote;

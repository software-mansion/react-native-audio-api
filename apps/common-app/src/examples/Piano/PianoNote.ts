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
/**
 * Above this cutoff the lowpass changes nothing audible, so loud notes skip it.
 * Every live key press is full velocity, and a filter per voice that does
 * nothing still costs render time.
 */
const audibleCutoffCeiling = 16000;
/** How quickly a still-ringing string is damped when its key is struck again. */
const restrikeDampTime = 0.015;

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

  const gain = audioContext.createGain();
  gain.gain.setValueAtTime(velocityToGain(velocity), when);

  const cutoff = velocityToCutoff(velocity);
  if (cutoff < audibleCutoffCeiling) {
    const tone = audioContext.createBiquadFilter();
    tone.type = 'lowpass';
    tone.Q.value = 0;
    tone.frequency.value = cutoff;
    source.connect(tone);
    tone.connect(gain);
  } else {
    source.connect(gain);
  }

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

/** Cuts a voice short, like a damper falling back onto the string. */
function dampVoice(voice: Voice, at: number) {
  voice.gain.gain.cancelAndHoldAtTime(at);
  voice.gain.gain.setTargetAtTime(0, at, restrikeDampTime);
  voice.stopsAt = at + restrikeDampTime * 6;
  voice.source.stop(voice.stopsAt);
}

/**
 * A key played live: it sounds on `start` and is released on `stop`. Striking
 * the key again damps the previous voice, as on a real piano, instead of
 * stacking another ringing voice on top of it.
 */
class PianoNote {
  /** The latest voice of this key, held down or still ringing after release. */
  private voice: Voice | null = null;
  private held = false;

  constructor(
    private readonly audioContext: AudioContext,
    private readonly midi: number
  ) {}

  start(samples: SampleMap) {
    const now = this.audioContext.currentTime;
    if (this.voice && (this.voice.stopsAt ?? Infinity) > now) {
      dampVoice(this.voice, now);
    }

    const voice = startVoice(this.audioContext, samples, this.midi, now);
    if (voice) {
      // Finished voices stay in the audio graph until disconnected, and keep
      // costing render time there.
      voice.source.onended = () => voice.gain.disconnect();
    }
    this.voice = voice;
    this.held = true;
  }

  stop() {
    if (!this.voice || !this.held) {
      return;
    }

    releaseVoice(this.voice, this.audioContext.currentTime);
    this.held = false;
  }
}

export default PianoNote;

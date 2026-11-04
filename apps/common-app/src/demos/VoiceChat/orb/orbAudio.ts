import { useCallback, useMemo } from 'react';
import { SharedValue, useSharedValue } from 'react-native-reanimated';

/**
 * Audio metrics that drive the orb, after rn-audio-shader's `audioMetrics.ts`.
 * The JS thread measures each chunk of samples; the orb's frame callback on the
 * UI thread picks up new readings and decays them between chunks.
 */

export type OrbVoice = 'user' | 'assistant';

/** Gain from RMS to the 0-1 level, per voice (from rn-audio-shader). */
const levelScale: Record<OrbVoice, number> = {
  user: 7.5,
  assistant: 5.6,
};

export type OrbReading = {
  level: number;
  peak: number;
  brightness: number;
  /** Bumped on every new reading, so the UI thread can tell it's fresh. */
  readId: number;
};

const silentReading: OrbReading = {
  level: 0,
  peak: 0,
  brightness: 0,
  readId: 0,
};

export type OrbReadings = Record<OrbVoice, SharedValue<OrbReading>>;

function measure(
  samples: Float32Array,
  sampleCount: number,
  scale: number,
  readId: number
): OrbReading {
  let sumSquares = 0;
  let peak = 0;
  let signChanges = 0;
  let previous = samples[0] ?? 0;
  for (let i = 0; i < sampleCount; i++) {
    const sample = samples[i];
    sumSquares += sample * sample;
    peak = Math.max(peak, Math.abs(sample));
    if (sample >= 0 !== previous >= 0) {
      signChanges += 1;
    }
    previous = sample;
  }
  const rms = Math.sqrt(sumSquares / Math.max(1, sampleCount));
  return {
    level: Math.min(1, rms * scale),
    peak: Math.min(1, peak * scale * 0.65),
    // Zero-crossing rate: a rough stand-in for how bright the voice sounds.
    brightness: Math.min(1, (signChanges / Math.max(1, sampleCount - 1)) * 18),
    readId,
  };
}

/** Latest readings for both voices, and a function to feed them samples. */
export function useOrbAudio() {
  const user = useSharedValue<OrbReading>(silentReading);
  const assistant = useSharedValue<OrbReading>(silentReading);
  const readings: OrbReadings = useMemo(
    () => ({ user, assistant }),
    [user, assistant]
  );

  const pushSamples = useCallback(
    (voice: OrbVoice, samples: Float32Array, sampleCount: number) => {
      const reading = readings[voice];
      reading.value = measure(
        samples,
        sampleCount,
        levelScale[voice],
        reading.value.readId + 1
      );
    },
    [readings]
  );

  return { readings, pushSamples };
}

/** Decayed state of one voice, owned by the orb's frame callback. */
export type OrbVoiceState = {
  level: number;
  peak: number;
  brightness: number;
  smoothedLevel: number;
  smoothedPeak: number;
  readId: number;
};

export function createVoiceState(): OrbVoiceState {
  'worklet';
  return {
    level: 0,
    peak: 0,
    brightness: 0,
    smoothedLevel: 0,
    smoothedPeak: 0,
    readId: 0,
  };
}

/**
 * Takes in a fresh reading if there is one, then decays the state by one frame.
 * The factors are per 60 Hz frame, scaled by the real frame time.
 */
export function stepVoiceState(
  state: OrbVoiceState,
  reading: OrbReading,
  frames: number
) {
  'worklet';
  if (reading.readId !== state.readId) {
    state.readId = reading.readId;
    state.level = reading.level;
    state.peak = Math.max(state.peak * 0.72, reading.peak);
    state.brightness = reading.brightness;
  }
  state.smoothedLevel +=
    (state.level - state.smoothedLevel) * (1 - Math.pow(1 - 0.36, frames));
  state.smoothedPeak = Math.max(
    state.peak,
    state.smoothedPeak * Math.pow(0.9, frames)
  );
  state.level *= Math.pow(0.88, frames);
  state.peak *= Math.pow(0.94, frames);
  state.brightness *= Math.pow(0.94, frames);
}

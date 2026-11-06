import { useSyncExternalStore } from 'react';
import type { AudioBuffer } from 'react-native-audio-api';

import { levelFromAmplitude } from './levels';

export interface Recording {
  id: number;
  title: string;
  createdAt: Date;
  /** Seconds. */
  duration: number;
  buffer: AudioBuffer;
  /** Loudness of `previewBarCount` equal slices, 0–1, for the row waveform. */
  preview: number[];
}

export const previewBarCount = 22;

/**
 * Recordings made since the app started, newest first. They live outside the
 * screen so leaving and reopening the demo keeps them.
 */
let recordings: Recording[] = [];
let recordingsMade = 0;
const listeners = new Set<() => void>();

function previewOf(buffer: AudioBuffer) {
  const samples = buffer.getChannelData(0);
  const sliceLength = Math.max(1, Math.floor(samples.length / previewBarCount));
  const preview: number[] = [];

  for (let bar = 0; bar < previewBarCount; bar += 1) {
    let peak = 0;
    const end = Math.min(samples.length, (bar + 1) * sliceLength);
    for (let i = bar * sliceLength; i < end; i += 1) {
      peak = Math.max(peak, Math.abs(samples[i]));
    }
    preview.push(levelFromAmplitude(peak));
  }

  return preview;
}

export function addRecording(buffer: AudioBuffer) {
  recordingsMade += 1;
  const recording: Recording = {
    id: recordingsMade,
    title: `Recording ${recordingsMade}`,
    createdAt: new Date(),
    duration: buffer.duration,
    buffer,
    preview: previewOf(buffer),
  };

  recordings = [recording, ...recordings];
  listeners.forEach((listener) => listener());
  return recording;
}

function subscribe(listener: () => void) {
  listeners.add(listener);
  return () => listeners.delete(listener);
}

export function useRecordings() {
  return useSyncExternalStore(subscribe, () => recordings);
}

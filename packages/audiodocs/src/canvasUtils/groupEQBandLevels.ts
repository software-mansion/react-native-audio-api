import AudioManager from "../audio/AudioManager";
import { frequencies } from "../audio/Equalizer";

function getBinIndexForFrequency(freq: number) {
  const nyquist = AudioManager.aCtx.sampleRate / 2;
  const binCount = AudioManager.analyser.fftSize / 2;

  return Math.round(freq / nyquist * binCount);
}

export default function groupEQBandLevels(fftOutput: Float32Array) {
  const bandLevels: number[] = [];

  for (let i = 0; i < frequencies.length - 1; i += 1) {
    const startFreq = frequencies[i];
    const endFrequency = frequencies[i + 1];

    const startIndex = getBinIndexForFrequency(startFreq);
    const endIndex = getBinIndexForFrequency(endFrequency);

    let sum = 0;

    for (let j = startIndex; j <= endIndex; j += 1) {
      sum += fftOutput[j];
    }

    const avgDb = sum / (endIndex - startIndex + 1);

    const clamped = Math.max(-12, Math.min(avgDb, 12));
    const norm = (clamped + 12) / 24;
    const visualValue = -12 + norm * 24;

    bandLevels.push(visualValue);
  }

  return bandLevels;
}

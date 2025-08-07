

import AudioManager from "../audio/AudioManager";

interface DataPoint {
  freq: number;
  value: number;
  posX: number;
}

const frequencies = [32, 100, 1000, 10000, 16000];
const bucketSizes = [5, 15, 20, 15];

const minFreq = frequencies[0];
const maxFreq = frequencies[frequencies.length - 1];
const logMin = Math.log10(minFreq);
const logMax = Math.log10(maxFreq);

export default function stretchFrequencies(fftOutput: Uint8Array, x: number, y: number, maxWidth: number, maxHeight: number) {
  const dataPoints: DataPoint[] = [];

  for (let rangeIndex = 0; rangeIndex < frequencies.length - 1; rangeIndex += 1) {
    const startFreq = frequencies[rangeIndex];
    const endFreq = frequencies[rangeIndex + 1];
    const currentBucketSize = bucketSizes[rangeIndex];

    for (let pointIndex = 0; pointIndex < currentBucketSize; pointIndex += 1) {
      const t = pointIndex / (currentBucketSize - 1);
      const frequency = startFreq + t * (endFreq - startFreq);

      const targetIndex = Math.round((frequency * (fftOutput.length - 1)) / AudioManager.aCtx.sampleRate);
      const clampedIndex = Math.max(0, Math.min(targetIndex, fftOutput.length - 1));
      const value = fftOutput[clampedIndex] || 0;

      const logFreq = Math.log10(frequency);
      const normalizedPos = (logFreq - logMin) / (logMax - logMin);
      const posX = x + (normalizedPos * maxWidth);

      dataPoints.push({ freq: frequency, value, posX });
    }
  }

  return dataPoints.map(p => ({
    x: p.posX,
    y: y + maxHeight - (p.value / 255 * maxHeight)
  }));
}

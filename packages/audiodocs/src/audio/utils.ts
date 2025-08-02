
export function downSampleLog(
    fft: Uint8Array,
    targetBucketCount: number,
    sampleRate: number,
    minFreq = 20
): number[] {
    const nyquist = sampleRate / 2;
    const binCount = fft.length;
    const maxFreq = nyquist;

    const buckets = new Array<number>(targetBucketCount).fill(0);
    const bucketCounts = new Array<number>(targetBucketCount).fill(0);

    for (let i = 0; i < binCount; i += 1) {
        const freq = (i / binCount) * nyquist;
        const safeFreq = Math.max(freq, minFreq);
        const norm = Math.log(safeFreq / minFreq) / Math.log(maxFreq / minFreq);
        const band = Math.max(
            0,
            Math.min(targetBucketCount - 1, Math.round(norm * (targetBucketCount - 1)))
        );

        buckets[band] += fft[i];
        bucketCounts[band] += 1;
    }

    for (let i = 0; i < targetBucketCount; i += 1) {
      if (bucketCounts[i] > 0) {
          buckets[i] /= bucketCounts[i];
      } else {
          buckets[i] = buckets[Math.max(0, i - 1)];
      }
    }

    return buckets;
}

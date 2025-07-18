
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

    for (let i = 0; i < binCount; i++) {
        const freq = (i / binCount) * nyquist;

        if (freq < minFreq) continue; // skip sub-audible bins

        const norm = Math.log(freq / minFreq) / Math.log(maxFreq / minFreq);
        const band = Math.min(
            targetBucketCount - 1,
            Math.floor(norm * targetBucketCount)
        );

        buckets[band] += fft[i];
        bucketCounts[band]++;
    }

    // Normalize by count
    for (let i = 0; i < targetBucketCount; i++) {
        if (bucketCounts[i] > 0) {
            buckets[i] /= bucketCounts[i];
        }
    }

    return buckets;
}

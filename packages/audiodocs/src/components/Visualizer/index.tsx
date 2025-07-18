import React, { useEffect, useRef } from 'react';

import AudioManager from '@site/src/audio/AudioManager';

 function downSampleLog(
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

const Visualizer: React.FC = () => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const analyser = AudioManager.analyser;
    const fftOutput = new Uint8Array(analyser.frequencyBinCount);
    const bucketCount = 28;

    // For slow emulation
    let slowBuckets = new Array(bucketCount).fill(0);
    let slowBucketsSeeds = new Array(bucketCount).fill(0).map(() => Math.random());
    const ghostSeeds = new Array(bucketCount).fill(0).map(() => Math.random() * 0.5 + 0.5);
    let slowPhase = 0;

    const draw = () => {
      requestAnimationFrame(draw);
      analyser.getByteFrequencyData(fftOutput);

      const drawingBuckets = downSampleLog(
        fftOutput,
        bucketCount,
        AudioManager.aCtx.sampleRate,
        80
      );

      ctx.fillStyle = 'rgba(255, 255, 255, 1)';
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      const bucketsToDraw = bucketCount / 2;
      const barWidth = canvas.width / (2 * bucketsToDraw + 1)
      const totalBarHeight = canvas.height;

      let barsToDraw = drawingBuckets;

      if (!AudioManager.isPlaying) {
        // Animate slow sine wave
        slowPhase += 0.0025;
        slowBuckets = slowBuckets.map((_, i) => {
          // Sine wave, slow phase
          return 60 + 40 * Math.sin(slowPhase + i * 0.05) * slowBucketsSeeds[i];
        });

        barsToDraw = slowBuckets;
      }

      for (let i = 0; i < bucketsToDraw; i += 1) {
        const value = barsToDraw[i + bucketCount / 4] || 0; // Skip first bin
        const height = Math.max(Math.min(1.2 * (value / 255) * totalBarHeight * ghostSeeds[i], totalBarHeight), 2);
        const offset = (canvas.height - height) / 2;
        const x = (i + 0.5) * barWidth * 2;

        // ctx.fillStyle = `#ffd2d7`;
        // ctx.fillStyle = '#33488e40';
        ctx.fillStyle = '#d0b2ff';
        ctx.fillRect(x, offset, barWidth, height);
      }

      for (let i = 0; i < bucketsToDraw; i += 1) {
        const value = barsToDraw[i + bucketCount / 4] || 0;
        const height = Math.max(0.8 * (value / 255) * totalBarHeight, 2);
        const offset = (canvas.height - height) / 2;
        const x = (i + 0.5) * barWidth * 2;

        // ctx.fillStyle = `#ff6259`;
        // ctx.fillStyle = '#001a72';
        ctx.fillStyle = '#b07eff';
        ctx.fillRect(x, offset, barWidth, height);
      }
    };

    draw();
  }, []);

  return (
    <canvas width={160} height={160} ref={canvasRef} />
  )
};


export default Visualizer;


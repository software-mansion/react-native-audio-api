import { useColorMode } from '@docusaurus/theme-common';
import React, { useEffect, useLayoutEffect, useRef, useState } from 'react';

import AudioManager from '@site/src/audio/AudioManager';
import { downSampleLog } from '@site/src/audio/utils';
import styles from './styles.module.css';
import { drawSpectroLines, getBarWidth, clearCanvas } from '@site/src/canvasUtils';

const barSpacing = 8.45;
const minFrequency = 80;

const Spectrogram: React.FC = () => {
  const { colorMode } = useColorMode();

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const canvasWrapperRef = useRef<HTMLDivElement>(null);

  const [isPlaying, setIsPlaying] = useState(false);
  const [width, setWidth] = useState(0);

  useLayoutEffect(() => {
    const canvas = canvasRef.current;

    if (!canvas || canvas.width === 0) {
      return;
    }

    const ctx = canvas.getContext('2d');

    if (!ctx) {
      return;
    }

    const analyser = AudioManager.analyser;
    const fftOutput = new Uint8Array(analyser.frequencyBinCount);
    const bucketCount = 64;

    const barWidth = getBarWidth(canvas.width, bucketCount, barSpacing);
    const totalBarHeight = canvas.height;

    const finalBucketCount = Math.floor(canvas.width / (barWidth + barSpacing));
    const bucketShift = 4;

    let slowPhase = 0;
    let slowBuckets = new Array(finalBucketCount + 2 * bucketShift).fill(0);
    let slowBucketsSeeds = new Array(finalBucketCount + 2 * bucketShift).fill(0).map(() => Math.random());

    function draw() {
      requestAnimationFrame(draw);
      analyser.getByteFrequencyData(fftOutput);

      const drawingBuckets = downSampleLog(
        fftOutput,
        finalBucketCount + 2 * bucketShift,
        AudioManager.aCtx.sampleRate,
        minFrequency
      );

      clearCanvas(canvas, ctx);
      let barsToDraw = drawingBuckets;

      if (!isPlaying) {
        slowPhase += 0.01;
        slowBuckets = slowBuckets.map((_, i) => {
          return 2 * (60 + 40 * Math.sin(slowPhase + i * 0.05) * slowBucketsSeeds[i]);
        });

        barsToDraw = slowBuckets;
      }

      drawSpectroLines(
        ctx,
        barsToDraw,
        finalBucketCount,
        bucketShift,
        totalBarHeight,
        barWidth,
        barSpacing,
        colorMode,
      );
    }

    draw();
  }, [width, isPlaying, colorMode]);

  useLayoutEffect(() => {
    const updateWidth = () => {
      if (canvasWrapperRef.current) {
        setWidth(canvasWrapperRef.current.clientWidth);
      }
    };

    updateWidth();
    window.addEventListener('resize', updateWidth);

    return () => {
      window.removeEventListener('resize', updateWidth);
    };
  }, []);

  useEffect(() => {
    AudioManager.addEventListener('playing', () => {
      setIsPlaying(true);
    });

    AudioManager.addEventListener('stopped', () => {
      setIsPlaying(false);
    });

    return () => {
      AudioManager.removeEventListener('playing', () => setIsPlaying(true));
      AudioManager.removeEventListener('stopped', () => setIsPlaying(false));
    };
  }, []);

  return (
    <div>
      <div ref={canvasWrapperRef}>
        <canvas className={styles.canvas} ref={canvasRef} width={width} height={250}></canvas>
      </div>
    </div>
  );
}

export default Spectrogram;

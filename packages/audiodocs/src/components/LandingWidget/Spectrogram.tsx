import { useColorMode } from '@docusaurus/theme-common';
import React, { useEffect, useLayoutEffect, useRef, useState } from 'react';

import AudioManager from '@site/src/audio/AudioManager';
import { downSampleLog } from '@site/src/audio/utils';
import styles from './styles.module.css';

const Spectrogram: React.FC = () => {
  const { colorMode } = useColorMode();

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const canvasWrapperRef = useRef<HTMLDivElement>(null);

  const [isPlaying, setIsPlaying] = useState(false);
  const [width, setWidth] = useState(0);

  useLayoutEffect(() => {
    const canvas = canvasRef.current;

    if (!canvas) {
      return;
    }

    const ctx = canvas.getContext('2d');

    if (!ctx) {
      return;
    }

    if (canvas.width == 0) {
      return;
    }

    const analyser = AudioManager.analyser;
    const fftOutput = new Uint8Array(analyser.frequencyBinCount);
    const bucketCount = 64;

    let slowPhase = 0;
    let slowBuckets = new Array(bucketCount).fill(0);
    let slowBucketsSeeds = new Array(bucketCount).fill(0).map(() => Math.random());

    function draw() {
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

      const bucketsToDraw = bucketCount;
      const barWidth = Math.max(20, canvas.width / (bucketsToDraw))
      const totalBarHeight = canvas.height;

      let barsToDraw = drawingBuckets;

      if (!isPlaying) {
        // Animate slow sine wave
        slowPhase += 0.0007;
        slowBuckets = slowBuckets.map((_, i) => {
          // Sine wave, slow phase
          return 2 * (60 + 40 * Math.sin(slowPhase + i * 0.05) * slowBucketsSeeds[i]);
        });

        barsToDraw = slowBuckets;
      }

      for (let i = 0; i < bucketsToDraw; i += 1) {
        const value = barsToDraw[i + bucketCount / 4] || 0;
        const height = Math.max((value / 255) * totalBarHeight, 2);
        const offset = canvas.height - height;
        const x =  (barWidth + 8.45) * (i);

        // Create gradient for each bar
        const gradient = ctx.createLinearGradient(0, offset + height, 0, offset);

        if (colorMode === 'dark') {
          gradient.addColorStop(0, '#FF6259');
          gradient.addColorStop(0.85, '#232736');
        } else {
          gradient.addColorStop(0, '#FF6259');
          gradient.addColorStop(0.85, '#FFD2D7');
        }

        ctx.fillStyle = gradient;
        ctx.fillRect(x, offset, barWidth, height);
      }
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
    <div className={styles.spectrogramContainer}>
      <div className={styles.spectrogramCanvasWrapper} ref={canvasWrapperRef}>
        <canvas className={styles.spectrogramCanvas} ref={canvasRef} width={width} height={250}></canvas>
      </div>
    </div>
  );
}

export default Spectrogram;

import React, { useCallback } from 'react';
import { useColorMode } from '@docusaurus/theme-common';

import Canvas, { CanvasContext } from '@site/src/components/Canvas';
import { downSampleLog } from '@site/src/audio/utils';
import AudioManager from '@site/src/audio/AudioManager';
import { drawSpectroLines, getBarWidth, clearCanvas } from '@site/src/canvasUtils';
import useIsPlaying from '@site/src/audio/useIsPlaying';

const barSpacing = 8.45;
const minFrequency = 80;

interface SpectrogramRenderingContext {
  analyser: AnalyserNode;
  fftOutput: Uint8Array;
  barWidth: number;
  bucketCount: number;
  totalBarHeight: number;
  slowPhase: number;
  slowBuckets: number[];
  slowBucketsSeeds: number[];
}


const Spectrogram: React.FC = () => {
  const { colorMode } = useColorMode();
  const isPlaying = useIsPlaying();

  const prepareRenderingContext = useCallback(({ canvas, ctx }: CanvasContext): SpectrogramRenderingContext => {
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

    return {
      analyser,
      fftOutput,
      bucketCount: finalBucketCount,
      totalBarHeight,
      barWidth,
      slowPhase,
      slowBuckets,
      slowBucketsSeeds,
    };
  }, []);

  const onDraw = useCallback(({ canvas, ctx }: CanvasContext, renderingContext: SpectrogramRenderingContext) => {
    const { analyser, fftOutput, bucketCount, totalBarHeight, barWidth, slowPhase, slowBuckets, slowBucketsSeeds } = renderingContext;

    analyser.getByteFrequencyData(fftOutput);

    const drawingBuckets = downSampleLog(
      fftOutput,
      bucketCount + 2 * 4,
      AudioManager.aCtx.sampleRate,
      minFrequency
    );

    clearCanvas(canvas, ctx);
    let barsToDraw = drawingBuckets;

    if (!isPlaying) {
      renderingContext.slowPhase += 0.01;
      renderingContext.slowBuckets.forEach((_, i) => {
        renderingContext.slowBuckets[i] = 2 * (60 + 40 * Math.sin(renderingContext.slowPhase + i * 0.05) * slowBucketsSeeds[i]);
      });

      barsToDraw = slowBuckets;
    }

    drawSpectroLines(
      ctx,
      barsToDraw,
      bucketCount,
      4,
      totalBarHeight,
      barWidth,
      barSpacing,
      colorMode,
    );
  }, [isPlaying, colorMode]);

  return (
    <Canvas<SpectrogramRenderingContext>
      onDraw={onDraw}
      prepareRenderingContext={prepareRenderingContext}
    />
  );
}

export default Spectrogram;

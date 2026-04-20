import { useColorMode } from '@docusaurus/theme-common';
import React, { useCallback, useRef } from 'react';

import AudioManager from '@site/src/audio/AudioManager';
import useEqualizerControls from '@site/src/audio/useEqualizerControls';
import useIsPlaying from '@site/src/audio/useIsPlaying';
import { createGradient, drawEQControlPoints, drawEqGrid, drawShadedCurve, getDrawingBounds, getEqualizerResponse, Point, stretchFrequencies } from '@site/src/canvasUtils';
import Canvas, { CanvasContext } from '../Canvas';
import styles from './styles.module.css';

interface EqRenderingContext {
  analyser: AnalyserNode;
  fftOutput: Uint8Array;
  eqPoints: Point[];
}

const Equalizer: React.FC = () => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  const isPlaying = useIsPlaying();
  const { colorMode } = useColorMode();

  const { equalizerBands } = useEqualizerControls(canvasRef);

  const prepareRenderingContext = useCallback(({ canvas }: CanvasContext): EqRenderingContext => {
    const analyser = AudioManager.analyser;
    const { width, height } = getDrawingBounds(canvas);

    return {
      analyser,
      fftOutput: new Uint8Array(analyser.frequencyBinCount),
      eqPoints: getEqualizerResponse(0, width, 0, height),
    };
  }, [equalizerBands]);

  const onDraw = useCallback(({ canvas, ctx }: CanvasContext, renderingContext: EqRenderingContext) => {
    const { analyser, fftOutput, eqPoints } = renderingContext;

    const cssWidth = Math.max(1, canvas.clientWidth || canvas.width);
    const cssHeight = Math.max(1, canvas.clientHeight || canvas.height);
    const scaleX = canvas.width / cssWidth;
    const scaleY = canvas.height / cssHeight;

    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.setTransform(scaleX, 0, 0, scaleY, 0, 0);

    const { x, y, width, height } = getDrawingBounds(canvas);
    const freqGradient = createGradient(ctx, 0, height, [250, 127, 124]); // #FA7F7C in RGB format
    const eqGradient = createGradient(ctx, 0, height, colorMode === 'dark' ? [171, 188, 245] : [0, 26, 114], 0.75); // #AAAAAA or #333333 in RGB format

    drawEqGrid(canvas, ctx);

    analyser.getByteFrequencyData(fftOutput);

    // Draw current output from the analyser
    const points = stretchFrequencies(fftOutput, x, y, width, height);
    drawShadedCurve(ctx, points, y, height, freqGradient, '#FA7F7C');

    // Draw equalizer control points and curve
    drawShadedCurve(ctx, eqPoints, y, height, eqGradient, colorMode === 'dark' ? '#ABBCF5' : '#001a72');
    drawEQControlPoints(canvas, ctx, x, y, width, height, colorMode === 'dark' ? '#ABBCF5' : '#001a72');
  }, [isPlaying, colorMode]);

  return (
    <Canvas onDraw={onDraw} prepareRenderingContext={prepareRenderingContext} ref={canvasRef} className={styles.canvas} />
  );
};

export default Equalizer;

import { useColorMode } from '@docusaurus/theme-common';
import React, { useCallback, useRef } from 'react';

import useIsPlaying from '@site/src/audio/useIsPlaying';
import Canvas, { CanvasContext } from '../Canvas';
import AudioManager from '@site/src/audio/AudioManager';
import { clearCanvas, createGradient, drawEqGrid, drawShadedCurve, stretchFrequencies, getEqualizerResponse, getDrawingBounds, Point, drawEQControlPoints } from '@site/src/canvasUtils';
import useEqualizerControls from '@site/src/audio/useEqualizerControls';
import styles from './styles.module.css';

interface EqRenderingContext {
  analyser: AnalyserNode;
  fftOutput: Uint8Array;
  freqGradient: CanvasGradient;
  eqGradient: CanvasGradient;
  eqPoints: Point[];
}

const Equalizer: React.FC = () => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  const isPlaying = useIsPlaying();
  const { colorMode } = useColorMode();

  const { equalizerBands } = useEqualizerControls(canvasRef);

  const prepareRenderingContext = useCallback(({ canvas, ctx }: CanvasContext): EqRenderingContext => {
    const analyser = AudioManager.analyser;
    const { width, height } = getDrawingBounds(canvas);

    return {
      analyser,
      fftOutput: new Uint8Array(analyser.frequencyBinCount),
      freqGradient: createGradient(ctx, 0, height, [250, 127, 124]), // #FA7F7C in RGB format
      eqGradient: createGradient(ctx, 0, height, colorMode === 'dark' ? [171, 188, 245] : [0, 26, 114], 0.75), // #AAAAAA or #333333 in RGB format
      eqPoints: getEqualizerResponse(0, width, 0, height),
    };
  }, [colorMode, equalizerBands]);

  const onDraw = useCallback(({ canvas, ctx }: CanvasContext, renderingContext: EqRenderingContext) => {
    const { analyser, fftOutput, freqGradient, eqGradient, eqPoints } = renderingContext;
    const { x, y, width, height } = getDrawingBounds(canvas);

    clearCanvas(canvas, ctx);
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

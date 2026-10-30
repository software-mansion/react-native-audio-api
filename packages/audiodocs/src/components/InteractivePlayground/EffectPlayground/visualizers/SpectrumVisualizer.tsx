import React, { FC, useCallback, useEffect, useRef } from 'react';
import { AnalyserNode } from 'react-native-audio-api';

import ResponsiveCanvas, { ResponsiveCanvasDrawParams } from '@site/src/ui/ResponsiveCanvas';

import type { FrequencyResponseReader } from '../types';

const MIN_FREQUENCY = 20;
const MAX_FREQUENCY = 20000;
const RESPONSE_POINTS = 160;
const MIN_DB = -30;
const MAX_DB = 30;
const AXIS_HEIGHT = 16;
const SPECTRUM_STEP_PX = 2;
const GRID_FREQUENCIES = [50, 100, 200, 500, 1000, 2000, 5000, 10000];
const AXIS_LABELS: Record<number, string> = { 100: '100 Hz', 1000: '1 kHz', 10000: '10 kHz' };

interface SpectrumVisualizerProps {
  analyser: AnalyserNode | null;
  /** Drawn on a ±30 dB scale over the spectrum, e.g. a filter's magnitude response. */
  frequencyResponse?: FrequencyResponseReader;
  sampleRate: number;
  theme: 'light' | 'dark';
}

interface ResponseBuffers {
  frequencies: Float32Array<ArrayBuffer>;
  magnitudes: Float32Array<ArrayBuffer>;
  phases: Float32Array<ArrayBuffer>;
}

function createResponseBuffers(): ResponseBuffers {
  const frequencies = new Float32Array(RESPONSE_POINTS);
  for (let i = 0; i < RESPONSE_POINTS; i++) {
    frequencies[i] = MIN_FREQUENCY * Math.pow(MAX_FREQUENCY / MIN_FREQUENCY, i / (RESPONSE_POINTS - 1));
  }
  return {
    frequencies,
    magnitudes: new Float32Array(RESPONSE_POINTS),
    phases: new Float32Array(RESPONSE_POINTS),
  };
}

function frequencyToX(frequency: number, width: number): number {
  return (Math.log(frequency / MIN_FREQUENCY) / Math.log(MAX_FREQUENCY / MIN_FREQUENCY)) * width;
}

function xToFrequency(x: number, width: number): number {
  return MIN_FREQUENCY * Math.pow(MAX_FREQUENCY / MIN_FREQUENCY, x / width);
}

export const SpectrumVisualizer: FC<SpectrumVisualizerProps> = ({
  analyser,
  frequencyResponse,
  sampleRate,
  theme,
}) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const animationFrameRef = useRef<number | null>(null);
  const spectrumRef = useRef<Uint8Array | null>(null);
  const responseBuffersRef = useRef<ResponseBuffers>(createResponseBuffers());

  const draw = useCallback(
    (canvas: HTMLCanvasElement) => {
      const context = canvas.getContext('2d');
      if (!context) {
        return;
      }

      const cssWidth = canvas.clientWidth || 300;
      const cssHeight = canvas.clientHeight || 150;
      const dpr = cssWidth > 0 && canvas.width > 0 ? canvas.width / cssWidth : window.devicePixelRatio || 1;
      const plotHeight = cssHeight - AXIS_HEIGHT;
      const isDark = theme === 'dark';

      context.setTransform(dpr, 0, 0, dpr, 0, 0);
      context.clearRect(0, 0, cssWidth, cssHeight);

      context.lineWidth = 1;
      context.strokeStyle = isDark ? 'rgba(252, 252, 255, 0.15)' : 'rgba(0, 0, 0, 0.15)';
      context.fillStyle = isDark ? 'rgba(252, 252, 255, 0.6)' : 'rgba(0, 0, 0, 0.55)';
      context.font = '11px sans-serif';
      context.textAlign = 'center';
      context.textBaseline = 'top';

      for (const frequency of GRID_FREQUENCIES) {
        const x = Math.round(frequencyToX(frequency, cssWidth)) + 0.5;
        context.beginPath();
        context.moveTo(x, 0);
        context.lineTo(x, plotHeight);
        context.stroke();

        const label = AXIS_LABELS[frequency];
        if (label) {
          context.fillText(label, x, plotHeight + 3);
        }
      }

      if (analyser) {
        const binCount = analyser.frequencyBinCount;
        if (!spectrumRef.current || spectrumRef.current.length !== binCount) {
          spectrumRef.current = new Uint8Array(binCount);
        }
        const spectrum = spectrumRef.current;
        analyser.getByteFrequencyData(spectrum);

        const binWidthHz = sampleRate / (binCount * 2);

        context.beginPath();
        context.moveTo(0, plotHeight);
        for (let x = 0; x <= cssWidth; x += SPECTRUM_STEP_PX) {
          const bin = Math.min(binCount - 1, Math.floor(xToFrequency(x, cssWidth) / binWidthHz));
          const level = spectrum[bin] / 255;
          context.lineTo(x, plotHeight - level * plotHeight);
        }
        context.lineTo(cssWidth, plotHeight);
        context.closePath();
        context.fillStyle = 'rgba(250, 127, 124, 0.35)';
        context.fill();
        context.strokeStyle = isDark ? '#ff7774' : '#fa7f7c';
        context.lineWidth = 1.5;
        context.stroke();
      }

      if (frequencyResponse) {
        const { frequencies, magnitudes, phases } = responseBuffersRef.current;
        frequencyResponse(frequencies, magnitudes, phases);

        const zeroDbY = plotHeight * (1 - (0 - MIN_DB) / (MAX_DB - MIN_DB));
        context.setLineDash([4, 4]);
        context.lineWidth = 1;
        context.strokeStyle = isDark ? 'rgba(252, 252, 255, 0.3)' : 'rgba(0, 0, 0, 0.3)';
        context.beginPath();
        context.moveTo(0, zeroDbY);
        context.lineTo(cssWidth, zeroDbY);
        context.stroke();
        context.setLineDash([]);
        context.textAlign = 'right';
        context.textBaseline = 'bottom';
        context.fillStyle = isDark ? 'rgba(252, 252, 255, 0.6)' : 'rgba(0, 0, 0, 0.55)';
        context.fillText('0 dB', cssWidth - 4, zeroDbY - 2);

        context.beginPath();
        for (let i = 0; i < RESPONSE_POINTS; i++) {
          const magnitude = magnitudes[i];
          const db = magnitude > 0 ? 20 * Math.log10(magnitude) : MIN_DB;
          const clampedDb = Math.min(MAX_DB, Math.max(MIN_DB, db));
          const x = frequencyToX(frequencies[i], cssWidth);
          const y = plotHeight * (1 - (clampedDb - MIN_DB) / (MAX_DB - MIN_DB));
          if (i === 0) {
            context.moveTo(x, y);
          } else {
            context.lineTo(x, y);
          }
        }
        context.strokeStyle = isDark ? '#fcfcff' : '#001a72';
        context.lineWidth = 2;
        context.stroke();
      }
    },
    [analyser, frequencyResponse, sampleRate, theme]
  );

  const handleCanvasDraw = useCallback(
    ({ canvas }: ResponsiveCanvasDrawParams) => {
      draw(canvas);
    },
    [draw]
  );

  useEffect(() => {
    const frame = () => {
      const canvas = canvasRef.current;
      if (canvas) {
        draw(canvas);
      }
      animationFrameRef.current = requestAnimationFrame(frame);
    };

    animationFrameRef.current = requestAnimationFrame(frame);

    return () => {
      if (animationFrameRef.current !== null) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [draw]);

  return <ResponsiveCanvas onDraw={handleCanvasDraw} canvasRef={canvasRef} throttleMs={16} />;
};

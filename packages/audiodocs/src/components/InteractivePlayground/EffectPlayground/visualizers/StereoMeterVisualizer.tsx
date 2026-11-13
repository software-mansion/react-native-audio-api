import React, { FC, useCallback, useEffect, useRef } from 'react';
import type { AudioContext, GainNode } from 'react-native-audio-api';

import ResponsiveCanvas, { ResponsiveCanvasDrawParams } from '@site/src/ui/ResponsiveCanvas';

import { ANALYSER_FFT_SIZE } from '../useEffectChain';

/**
 * The web wrappers expose the browser objects they decorate. The meter taps those directly
 * because it is documentation plumbing, not part of the example the reader follows, and the
 * wrapper build shipped with the docs does not always cover ChannelSplitterNode.
 */
interface BrowserBackedContext {
  context: globalThis.AudioContext;
}

interface BrowserBackedNode {
  node: globalThis.AudioNode;
}

const METER_FLOOR_DB = -60;
const METER_DECAY_PER_FRAME = 0.92;
const BAR_HEIGHT = 28;
const BAR_GAP = 20;
const LABEL_WIDTH = 24;

interface StereoMeterVisualizerProps {
  ctx: AudioContext | null;
  /** Node whose output is measured; the meter taps it without altering the graph's routing. */
  monitor: GainNode | null;
  theme: 'light' | 'dark';
}

interface ChannelAnalysers {
  left: globalThis.AnalyserNode;
  right: globalThis.AnalyserNode;
}

function rmsLevel(samples: Float32Array): number {
  let sum = 0;
  for (let i = 0; i < samples.length; i++) {
    sum += samples[i] * samples[i];
  }
  const rms = Math.sqrt(sum / samples.length);
  if (rms <= 0) {
    return 0;
  }
  const db = 20 * Math.log10(rms);
  return Math.min(1, Math.max(0, (db - METER_FLOOR_DB) / -METER_FLOOR_DB));
}

export const StereoMeterVisualizer: FC<StereoMeterVisualizerProps> = ({ ctx, monitor, theme }) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const animationFrameRef = useRef<number | null>(null);
  const analysersRef = useRef<ChannelAnalysers | null>(null);
  const samplesRef = useRef(new Float32Array(ANALYSER_FFT_SIZE));
  const displayedLevelsRef = useRef({ left: 0, right: 0 });

  useEffect(() => {
    if (!ctx || !monitor) {
      return undefined;
    }

    const browserContext = (ctx as unknown as BrowserBackedContext).context;
    const browserMonitor = (monitor as unknown as BrowserBackedNode).node;
    const splitter = browserContext.createChannelSplitter(2);
    const left = browserContext.createAnalyser();
    const right = browserContext.createAnalyser();
    browserMonitor.connect(splitter);
    splitter.connect(left, 0);
    splitter.connect(right, 1);
    analysersRef.current = { left, right };

    return () => {
      analysersRef.current = null;
      try {
        browserMonitor.disconnect(splitter);
        splitter.disconnect();
      } catch {
        // The context may already be closed; nothing is left to tear down then.
      }
    };
  }, [ctx, monitor]);

  const draw = useCallback(
    (canvas: HTMLCanvasElement) => {
      const context = canvas.getContext('2d');
      if (!context) {
        return;
      }

      const cssWidth = canvas.clientWidth || 300;
      const cssHeight = canvas.clientHeight || 150;
      const dpr = cssWidth > 0 && canvas.width > 0 ? canvas.width / cssWidth : window.devicePixelRatio || 1;
      const isDark = theme === 'dark';

      const analysers = analysersRef.current;
      const levels = displayedLevelsRef.current;
      const samples = samplesRef.current;

      const measure = (analyser: globalThis.AnalyserNode | undefined) => {
        if (!analyser) {
          return 0;
        }
        analyser.getFloatTimeDomainData(samples);
        return rmsLevel(samples);
      };

      levels.left = Math.max(measure(analysers?.left), levels.left * METER_DECAY_PER_FRAME);
      levels.right = Math.max(measure(analysers?.right), levels.right * METER_DECAY_PER_FRAME);

      context.setTransform(dpr, 0, 0, dpr, 0, 0);
      context.clearRect(0, 0, cssWidth, cssHeight);

      const trackWidth = cssWidth - LABEL_WIDTH;
      const totalHeight = BAR_HEIGHT * 2 + BAR_GAP;
      const top = (cssHeight - totalHeight) / 2;

      context.font = '600 13px sans-serif';
      context.textBaseline = 'middle';
      context.textAlign = 'left';

      const drawBar = (label: string, level: number, y: number) => {
        context.fillStyle = isDark ? 'rgba(252, 252, 255, 0.8)' : 'rgba(0, 0, 0, 0.7)';
        context.fillText(label, 0, y + BAR_HEIGHT / 2);

        context.fillStyle = 'rgba(145, 159, 207, 0.2)';
        context.fillRect(LABEL_WIDTH, y, trackWidth, BAR_HEIGHT);

        const gradient = context.createLinearGradient(LABEL_WIDTH, 0, cssWidth, 0);
        gradient.addColorStop(0, isDark ? '#ff7774' : '#fa7f7c');
        gradient.addColorStop(1, isDark ? '#c7413e' : '#e0413d');
        context.fillStyle = gradient;
        context.fillRect(LABEL_WIDTH, y, trackWidth * level, BAR_HEIGHT);
      };

      drawBar('L', levels.left, top);
      drawBar('R', levels.right, top + BAR_HEIGHT + BAR_GAP);
    },
    [theme]
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

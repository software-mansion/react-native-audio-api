import React, { useCallback, useEffect, useRef, useState } from 'react';
import type { AnalyserNode } from 'react-native-audio-api';

import Canvas from './Canvas';
import Charts from './Charts';

const FRAME_INTERVAL_MS = 20;

interface FreqTimeChartProps {
  analyser: AnalyserNode;
  isPlaying: boolean;
  fftSize: number;
  frequencyBinCount: number;
}

const FreqTimeChart: React.FC<FreqTimeChartProps> = (props) => {
  const { analyser, isPlaying, fftSize, frequencyBinCount } = props;

  const timeBufferRef = useRef(new Uint8Array(fftSize).fill(127));
  const freqBufferRef = useRef(new Uint8Array(frequencyBinCount).fill(0));
  const rafRef = useRef<number | null>(null);
  const lastFrameAtRef = useRef(0);

  const [timeData, setTimeData] = useState(
    () => new Uint8Array(fftSize).fill(127)
  );
  const [frequencyData, setFrequencyData] = useState(
    () => new Uint8Array(frequencyBinCount).fill(0)
  );

  const publishAnalyserData = useCallback(() => {
    const timeBuffer = timeBufferRef.current;
    const freqBuffer = freqBufferRef.current;

    analyser.getByteTimeDomainData(timeBuffer);
    analyser.getByteFrequencyData(freqBuffer);

    setTimeData(new Uint8Array(timeBuffer));
    setFrequencyData(new Uint8Array(freqBuffer));
  }, [analyser]);

  const draw = useCallback(() => {
    const now = performance.now();

    if (now - lastFrameAtRef.current >= FRAME_INTERVAL_MS) {
      lastFrameAtRef.current = now;
      publishAnalyserData();
    }

    rafRef.current = requestAnimationFrame(draw);
  }, [publishAnalyserData]);

  useEffect(() => {
    if (!isPlaying) {
      if (rafRef.current !== null) {
        cancelAnimationFrame(rafRef.current);
        rafRef.current = null;
      }
      return;
    }

    lastFrameAtRef.current = 0;
    publishAnalyserData();
    rafRef.current = requestAnimationFrame(draw);

    return () => {
      if (rafRef.current !== null) {
        cancelAnimationFrame(rafRef.current);
        rafRef.current = null;
      }
    };
  }, [draw, isPlaying, publishAnalyserData]);

  return (
    <Canvas>
      <Charts.TimeChart
        timeData={timeData}
        frequencyData={frequencyData}
        fftSize={fftSize}
        frequencyBinCount={frequencyBinCount}
      />
      <Charts.FrequencyChart
        timeData={timeData}
        frequencyData={frequencyData}
        fftSize={fftSize}
        frequencyBinCount={frequencyBinCount}
      />
    </Canvas>
  );
};

export default FreqTimeChart;

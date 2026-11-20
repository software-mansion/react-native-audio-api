import React from 'react';
import type { SharedValue } from 'react-native-reanimated';

import Canvas from './Canvas';
import Charts from './Charts';

interface FreqTimeChartProps {
  timeDataSV: SharedValue<Uint8Array<ArrayBuffer>>;
  frequencyDataSV: SharedValue<Uint8Array<ArrayBuffer>>;
  fftSize: number;
  frequencyBinCount: number;
}

const FreqTimeChart: React.FC<FreqTimeChartProps> = (props) => {
  const { timeDataSV, frequencyDataSV, fftSize, frequencyBinCount } = props;

  return (
    <Canvas>
      <Charts.TimeChart
        timeDataSV={timeDataSV}
        frequencyDataSV={frequencyDataSV}
        fftSize={fftSize}
        frequencyBinCount={frequencyBinCount}
      />
      <Charts.FrequencyChart
        timeDataSV={timeDataSV}
        frequencyDataSV={frequencyDataSV}
        fftSize={fftSize}
        frequencyBinCount={frequencyBinCount}
      />
    </Canvas>
  );
};

export default FreqTimeChart;

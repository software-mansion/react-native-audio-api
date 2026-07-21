import React from 'react';
import type { SharedValue } from 'react-native-reanimated';

import Canvas from './Canvas';
import Charts from './Charts';

interface FreqTimeChartProps {
  timeDataSV: SharedValue<Uint8Array>;
  timeDataTickSV: SharedValue<number>;
  frequencyDataSV: SharedValue<Uint8Array>;
  frequencyDataTickSV: SharedValue<number>;
  fftSize: number;
  frequencyBinCount: number;
}

const FreqTimeChart: React.FC<FreqTimeChartProps> = (props) => {
  const {
    timeDataSV,
    timeDataTickSV,
    frequencyDataSV,
    frequencyDataTickSV,
    fftSize,
    frequencyBinCount,
  } = props;

  return (
    <Canvas>
      <Charts.TimeChart
        timeDataSV={timeDataSV}
        timeDataTickSV={timeDataTickSV}
        frequencyDataSV={frequencyDataSV}
        frequencyDataTickSV={frequencyDataTickSV}
        fftSize={fftSize}
        frequencyBinCount={frequencyBinCount}
      />
      <Charts.FrequencyChart
        timeDataSV={timeDataSV}
        timeDataTickSV={timeDataTickSV}
        frequencyDataSV={frequencyDataSV}
        frequencyDataTickSV={frequencyDataTickSV}
        fftSize={fftSize}
        frequencyBinCount={frequencyBinCount}
      />
    </Canvas>
  );
};

export default FreqTimeChart;

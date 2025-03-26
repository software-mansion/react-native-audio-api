import React from 'react';

import { withCanvas } from './Canvas';
import Charts from './Charts';

interface FreqTimeChartProps {
  timeData: Uint8Array;
  frequencyData: Uint8Array;
  fftSize: number;
  frequencyBinCount: number;
}

const FreqTimeChart: React.FC<FreqTimeChartProps> = (props) => {
  const { timeData, frequencyData, fftSize, frequencyBinCount } = props;

  return (
    <>
      <Charts.TimeChart
        data={timeData}
        fftSize={fftSize}
        frequencyBinCount={frequencyBinCount}
      />
      <Charts.FrequencyChart
        data={frequencyData}
        fftSize={fftSize}
        frequencyBinCount={frequencyBinCount}
      />
    </>
  );
};

export default withCanvas(FreqTimeChart);

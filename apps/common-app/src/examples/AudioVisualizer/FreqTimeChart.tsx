import React from 'react';

import { withCanvas } from './Canvas';
import Charts from './Charts';

interface FreqTimeChartProps {
  timeData: number[];
  frequencyData: number[];
  frequencyBinCount: number;
}

const FreqTimeChart: React.FC<FreqTimeChartProps> = (props) => {
  const { timeData, frequencyData, frequencyBinCount } = props;

  return (
    <>
      <Charts.TimeChart data={timeData} frequencyBinCount={frequencyBinCount} />
      <Charts.FrequencyChart
        data={frequencyData}
        frequencyBinCount={frequencyBinCount}
      />
    </>
  );
};

export default withCanvas(FreqTimeChart);

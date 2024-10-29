import React, { memo } from 'react';
import { StyleSheet } from 'react-native';
import { Line, Circle, Paint, vec } from '@shopify/react-native-skia';

import { colors } from '../../styles';
import { numBeats, cPoint, maxSize, buttonRadius } from './constants';
import { getAngle, getPointCX, getPointCY } from './utils';
import type { Instrument } from './types';
import instruments from './instruments';

const points = Array(numBeats).fill(0);

const Grid: React.FC = () => {
  const renderLine = (index: number) => {
    const angle = getAngle(index);
    const x = getPointCX(angle, maxSize / 2, cPoint.x);
    const y = getPointCY(angle, maxSize / 2, cPoint.y);

    return (
      <Line
        key={`ine-${index}`}
        p1={vec(cPoint.x, cPoint.y)}
        p2={vec(x, y)}
        strokeWidth={StyleSheet.hairlineWidth}
        color={colors.border}
      />
    );
  };

  const renderTouchPoint = (instrument: Instrument, index: number) => {
    const angle = getAngle(index);
    const x = getPointCX(angle, instrument.radius, cPoint.x);
    const y = getPointCY(angle, instrument.radius, cPoint.y);

    return (
      <Circle
        cx={x}
        cy={y}
        r={buttonRadius}
        color={colors.border}
        key={`${instrument.name}-${index}`}
      />
    );
  };

  const renderInstrument = (instrument: Instrument) => (
    <React.Fragment key={instrument.name}>
      <Circle
        cx={cPoint.x}
        cy={cPoint.y}
        color="transparent"
        r={instrument.radius}
      >
        <Paint
          style="stroke"
          color={colors.border}
          strokeWidth={StyleSheet.hairlineWidth}
        />
      </Circle>
      {points.map((_, index) => renderTouchPoint(instrument, index))}
    </React.Fragment>
  );

  return (
    <>
      {points.map((_, index) => renderLine(index))}
      {instruments.map(renderInstrument)}
    </>
  );
};

export default memo(Grid);

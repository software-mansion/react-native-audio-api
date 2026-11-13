import React, { memo } from 'react';
import { StyleSheet } from 'react-native';
import { Circle, Paint } from '@shopify/react-native-skia';

import { numBeats, cPoint, inactiveStepRadius } from './constants';
import { getAngle, getPointCX, getPointCY } from '../../utils/skiUtils';
import type { Instrument } from '../../types';
import instruments from './instruments';

const ringColor = 'rgba(238, 240, 255, 0.12)';
const inactiveStepColor = 'rgba(238, 240, 255, 0.14)';

const steps = Array(numBeats).fill(0);

/**
 * The static part of the sequencer: one ring per instrument with a faint dot
 * per step.
 */
const Grid: React.FC = () => {
  const renderStep = (instrument: Instrument, index: number) => {
    const angle = getAngle(index, numBeats);

    return (
      <Circle
        key={`${instrument.name}-${index}`}
        cx={getPointCX(angle, instrument.radius, cPoint.x)}
        cy={getPointCY(angle, instrument.radius, cPoint.y)}
        r={inactiveStepRadius}
        color={inactiveStepColor}
      />
    );
  };

  return (
    <>
      {instruments.map((instrument) => (
        <React.Fragment key={instrument.name}>
          <Circle
            cx={cPoint.x}
            cy={cPoint.y}
            r={instrument.radius}
            color="transparent"
          >
            <Paint
              style="stroke"
              color={ringColor}
              strokeWidth={StyleSheet.hairlineWidth * 2}
            />
          </Circle>
          {steps.map((_, index) => renderStep(instrument, index))}
        </React.Fragment>
      ))}
    </>
  );
};

export default memo(Grid);

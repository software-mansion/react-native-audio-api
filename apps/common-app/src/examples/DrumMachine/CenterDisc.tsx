import React, { memo } from 'react';
import { Circle } from '@shopify/react-native-skia';

import { colors } from '../../styles';
import { centerDiscRadius, cPoint } from './constants';

/**
 * Masks the pattern polygons in the middle so the tempo readout sits on a clean
 * background.
 */
const CenterDisc: React.FC = () => (
  <Circle
    cx={cPoint.x}
    cy={cPoint.y}
    r={centerDiscRadius}
    color={colors.background}
  />
);

export default memo(CenterDisc);

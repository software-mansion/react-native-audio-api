import React from 'react';
import Svg, { Circle, Path } from 'react-native-svg';

type Props = {
  selected: boolean;
  size?: number;
  color?: string;
};

const CheckCircleIcon: React.FC<Props> = ({ selected, size = 24, color = '#FFFFFF' }) => {
  return (
    <Svg width={size} height={size} viewBox="0 0 24 24" fill="none">
      <Circle cx="12" cy="12" r="10" stroke={color} strokeWidth="2" fill="none" />
      {selected ?
        <Path
          d="M16 8L10.5 14L8 11.5"
          stroke={color}
          strokeWidth="2"
          strokeLinecap="round"
          strokeLinejoin="round"
        />
        : null}
    </Svg>
  );
};

export default CheckCircleIcon;

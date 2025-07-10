import React from 'react';
import Svg, { Line } from 'react-native-svg';

type Props = {
  size?: number;
  color?: string;
};

const MenuIcon: React.FC<Props> = ({ size = 24, color = '#FFFFFF' }) => {
  const strokeWidth = 2;
  const spacing = size / 4;
  const linesY = [spacing, size / 2, size - spacing];

  return (
    <Svg width={size} height={size} viewBox={`0 0 ${size} ${size}`} fill="none">
      {linesY.map((y, index) => (
        <Line
          key={index}
          x1={0}
          y1={y}
          x2={size}
          y2={y}
          stroke={color}
          strokeWidth={strokeWidth}
          strokeLinecap="round"
        />
      ))}
    </Svg>
  );
};

export default MenuIcon;

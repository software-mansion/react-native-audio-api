import React from 'react';
import { View } from 'react-native';
import Svg, { Path, Rect } from 'react-native-svg';

type Props = {
  isPlaying: boolean;
  size?: number;
  color?: string;
};

const PlayPauseIcon: React.FC<Props> = ({
  isPlaying = true,
  size = 48,
  color = '#FFFFFF',
}) => {
  return (
    <View>
      <Svg width={size} height={size} viewBox="0 0 64 64" fill="none">
        {isPlaying ? (
          <>
            <Rect x="16" y="12" width="10" height="40" rx="2" fill={color} />
            <Rect x="38" y="12" width="10" height="40" rx="2" fill={color} />
          </>
        ) : (
          <Path d="M18 12L52 32L18 52V12Z" fill={color} />
        )}
      </Svg>
    </View>
  );
};

export default PlayPauseIcon;

import React from 'react';
import { StyleSheet } from 'react-native';
import { useDerivedValue, useSharedValue } from 'react-native-reanimated';
import { vec, Rect, Canvas, RadialGradient } from '@shopify/react-native-skia';

const BGGradient = () => {
  const canvasSize = useSharedValue({ width: 0, height: 0 });

  const gradientRadius = useDerivedValue(() => canvasSize.value.width);
  const gradientC = useDerivedValue(() => vec(canvasSize.value.width / 2, 0));

  return (
    <Canvas style={styles.canvas} onSize={canvasSize}>
      <Rect
        x={0}
        y={0}
        width={canvasSize.value.width}
        height={canvasSize.value.height}
      >
        <RadialGradient
          c={gradientC}
          r={gradientRadius}
          colors={['#333', '#222']}
        />
      </Rect>
    </Canvas>
  );
};

export default BGGradient;

const styles = StyleSheet.create({
  canvas: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
  },
});

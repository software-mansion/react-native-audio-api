import React, { useCallback, useState } from 'react';
import { LayoutChangeEvent, StyleSheet, View } from 'react-native';
import { vec, Rect, Canvas, RadialGradient } from '@shopify/react-native-skia';

import { colors } from '../styles';

const BGGradient = () => {
  const [size, setSize] = useState({ width: 0, height: 0 });

  const onWrapperLayout = useCallback((event: LayoutChangeEvent) => {
    setSize({
      width: event.nativeEvent.layout.width,
      height: event.nativeEvent.layout.height,
    });
  }, []);

  return (
    <View style={styles.wrapper} onLayout={onWrapperLayout}>
      <Canvas style={styles.canvas}>
        <Rect x={0} y={0} width={size.width} height={size.height}>
          <RadialGradient
            r={size.width}
            c={vec(size.width / 2, 0)}
            colors={[colors.backgroundLight, colors.background]}
          />
        </Rect>
      </Canvas>
    </View>
  );
};

export default BGGradient;

const styles = StyleSheet.create({
  wrapper: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
  },
  canvas: {
    width: '100%',
    height: '100%',
  },
});

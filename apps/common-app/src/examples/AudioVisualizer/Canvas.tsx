import React, {
  useContext,
  createContext,
  PropsWithChildren,
  useMemo,
  useState,
} from 'react';
import { LayoutChangeEvent, StyleSheet, View } from 'react-native';
import { Canvas as SKCanvas } from '@shopify/react-native-skia';

interface Size {
  width: number;
  height: number;
}

interface CanvasContext {
  ready: boolean;
  size: Size;
}

const CanvasContext = createContext<CanvasContext>({
  ready: false,
  size: { width: 0, height: 0 },
});

const Canvas: React.FC<PropsWithChildren> = ({ children }) => {
  const [squareSide, setSquareSide] = useState(0);
  const [renderSize, setRenderSize] = useState<Size>({ width: 0, height: 0 });

  const ready = squareSide > 0;

  const context = useMemo(
    () => ({
      ready,
      size: renderSize,
    }),
    [ready, renderSize]
  );

  const handleWrapperLayout = (event: LayoutChangeEvent) => {
    const { width, height } = event.nativeEvent.layout;

    if (width <= 0 || height <= 0) {
      return;
    }

    const side = Math.min(width, height);

    setSquareSide(side);
    setRenderSize({ width: side, height: side });
  };

  return (
    <View style={styles.wrapper} onLayout={handleWrapperLayout}>
      {ready ? (
        <SKCanvas style={{ width: squareSide, height: squareSide }}>
          <CanvasContext.Provider value={context}>{children}</CanvasContext.Provider>
        </SKCanvas>
      ) : null}
    </View>
  );
};

export function useCanvas() {
  const canvasContext = useContext(CanvasContext);

  if (!canvasContext.ready) {
    throw new Error('Canvas is not ready');
  }

  return canvasContext;
}

export default Canvas;

const styles = StyleSheet.create({
  wrapper: {
    flex: 1,
    width: '100%',
    alignSelf: 'stretch',
    justifyContent: 'center',
    alignItems: 'center',
  },
});

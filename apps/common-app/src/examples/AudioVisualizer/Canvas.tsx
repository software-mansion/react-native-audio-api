import React, {
  useContext,
  createContext,
  PropsWithChildren,
  useMemo,
} from 'react';
import { StyleSheet } from 'react-native';
import { Canvas as SKCanvas, useCanvasSize } from '@shopify/react-native-skia';

interface Size {
  width: number;
  height: number;
}

interface CanvasContext {
  initialized: boolean;
  size: Size;
}

const CanvasContext = createContext<CanvasContext>({
  initialized: false,
  size: { width: 0, height: 0 },
});

const Canvas: React.FC<PropsWithChildren> = ({ children }) => {
  const { ref, size } = useCanvasSize();

  const context = useMemo(
    () => ({
      initialized: true,
      size: { width: size.width, height: size.height },
    }),
    [size]
  );

  return (
    <SKCanvas style={styles.canvas} ref={ref}>
      <CanvasContext.Provider value={context}>
        {children}
      </CanvasContext.Provider>
    </SKCanvas>
  );
};

export function useCanvas() {
  const canvasContext = useContext(CanvasContext);

  if (!canvasContext.initialized) {
    throw new Error('Canvas context not initialized');
  }

  return canvasContext;
}

export function withCanvas<P extends object>(
  Component: React.ComponentType<P>
) {
  return (props: P) => {
    return (
      <Canvas>
        <Component {...props} />
      </Canvas>
    );
  };
}

export default Canvas;

const styles = StyleSheet.create({
  canvas: {
    flex: 1,
  },
});

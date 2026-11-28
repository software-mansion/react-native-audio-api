import {
  isLiquidGlassSupported,
  LiquidGlassView,
} from '@callstack/liquid-glass';
import React, { ComponentProps, FC } from 'react';

import { colors } from '../styles';

type GlassSurfaceProps = ComponentProps<typeof LiquidGlassView> & {
  /** Shows the pressed fill where liquid glass is unavailable. */
  pressed?: boolean;
  /** Fallback fill for a surface that carries a state colour. */
  fallbackColor?: string;
};

/**
 * Dark liquid glass. Before iOS 26 (and on Android) it falls back to a plain
 * `surface` fill.
 */
const GlassSurface: FC<GlassSurfaceProps> = ({
  style,
  pressed = false,
  fallbackColor,
  ...props
}) => (
  <LiquidGlassView
    colorScheme="dark"
    {...props}
    style={[
      style,
      !isLiquidGlassSupported && {
        backgroundColor:
          fallbackColor ?? (pressed ? colors.surfacePressed : colors.surface),
      },
    ]}
  />
);

export default GlassSurface;

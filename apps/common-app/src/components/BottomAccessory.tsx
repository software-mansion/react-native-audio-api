import {
  isLiquidGlassSupported,
  LiquidGlassView,
} from '@callstack/liquid-glass';
import React, { FC, PropsWithChildren } from 'react';
import { StyleSheet } from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import { colors, layout } from '../styles';

/** Height of the capsule, matching the iOS 26 tab bar accessory. */
export const bottomAccessoryHeight = 52;
const sideInset = 20;
const bottomGap = 8;

/**
 * A liquid-glass capsule floating above the home indicator, shaped like the iOS
 * 26 tab bar accessory. Stack screens have no tab bar to host the system
 * accessory, so they use this instead. Render it last in the screen so it sits
 * over the content; `useBottomAccessorySpace` gives the room to leave below
 * content.
 */
const BottomAccessory: FC<PropsWithChildren> = ({ children }) => {
  const insets = useSafeAreaInsets();

  return (
    <LiquidGlassView
      colorScheme="dark"
      style={[
        styles.capsule,
        { bottom: insets.bottom + bottomGap },
        !isLiquidGlassSupported && styles.fallback,
      ]}
    >
      {children}
    </LiquidGlassView>
  );
};

/** Bottom padding that keeps screen content clear of the accessory. */
export function useBottomAccessorySpace() {
  const insets = useSafeAreaInsets();
  return insets.bottom + bottomGap + bottomAccessoryHeight + layout.spacing;
}

export default BottomAccessory;

const styles = StyleSheet.create({
  capsule: {
    position: 'absolute',
    left: sideInset,
    right: sideInset,
    height: bottomAccessoryHeight,
    borderRadius: bottomAccessoryHeight / 2,
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 8,
    gap: 12,
  },
  fallback: {
    backgroundColor: colors.surface,
  },
});

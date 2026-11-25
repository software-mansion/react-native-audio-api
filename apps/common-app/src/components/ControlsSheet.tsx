import { BottomSheet, type Detent } from '@swmansion/react-native-bottom-sheet';
import React, { PropsWithChildren, useMemo } from 'react';
import { LayoutChangeEvent, StyleSheet, View } from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import { layout } from '../styles';
import GlassSurface from './GlassSurface';

const sideGap = 8;
const bottomGap = 8;

type ControlsSheetProps = PropsWithChildren<{
  /**
   * Space the card and its gap cover above the bottom safe area (which
   * `Container` already keeps clear), so content can pad itself clear of it.
   */
  onCoveredHeightChange?: (height: number) => void;
}>;

/**
 * A screen's always-visible controls: a liquid glass card floating above the
 * home indicator, sized to its content. Its only detent is the content height,
 * so it has no closed position, nothing to drag and no handle. Render it last
 * inside the screen's root view; it overlays the screen without blocking
 * touches outside the card.
 */
const ControlsSheet: React.FC<ControlsSheetProps> = ({
  onCoveredHeightChange,
  children,
}) => {
  const insets = useSafeAreaInsets();
  const bottomInset = insets.bottom + bottomGap;

  const detents = useMemo<Detent[]>(
    () => [
      {
        value: 'content',
        insets: { left: sideGap, right: sideGap, bottom: bottomInset },
      },
    ],
    [bottomInset]
  );

  const onContentLayout = (event: LayoutChangeEvent) =>
    onCoveredHeightChange?.(event.nativeEvent.layout.height + bottomGap);

  return (
    <BottomSheet
      index={0}
      detents={detents}
      surface={<GlassSurface style={StyleSheet.absoluteFill} />}
      surfaceStyle={styles.surface}
    >
      <View onLayout={onContentLayout}>{children}</View>
    </BottomSheet>
  );
};

export default ControlsSheet;

const styles = StyleSheet.create({
  surface: {
    borderRadius: layout.cardRadius,
    borderCurve: 'continuous',
  },
});

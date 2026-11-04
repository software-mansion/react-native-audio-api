import { useNavigation, useNavigationState } from '@react-navigation/native';
import { PropsWithChildren, useEffect } from 'react';
import { Platform, View } from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import type { ControlsSheetKey, ControlsSheetOwner } from './useControlsSheet';

type ControlsSheetLayoutProps = PropsWithChildren<{
  sheet: ControlsSheetKey;
  owner: ControlsSheetOwner;
}>;

/**
 * Keeps a controls sheet tied to its screen. A native back press or swipe pops
 * only the owner screen, which would leave the sheet route (stacked above it)
 * presented over whatever comes next, so the sheet removes itself once its
 * owner is no longer in the stack.
 *
 * On Android the sheet is drawn edge-to-edge, so its content is padded above
 * the system navigation bar. The owners pad themselves by the measured content
 * height only, which lines up because their `Container` already excludes that
 * bar.
 */
export function ControlsSheetLayout({
  sheet,
  owner,
  children,
}: ControlsSheetLayoutProps) {
  const navigation = useNavigation();
  const insets = useSafeAreaInsets();
  const isOwnerInStack = useNavigationState(sheet, (state) =>
    state.routes.some((route) => route.name === owner)
  );

  useEffect(() => {
    if (!isOwnerInStack) {
      navigation.goBack();
    }
  }, [isOwnerInStack, navigation]);

  const bottomInset = Platform.OS === 'android' ? insets.bottom : 0;

  return <View style={{ paddingBottom: bottomInset }}>{children}</View>;
}

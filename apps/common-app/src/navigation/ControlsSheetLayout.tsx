import { useNavigation, useNavigationState } from '@react-navigation/native';
import { PropsWithChildren, useEffect } from 'react';

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
 */
export function ControlsSheetLayout({
  sheet,
  owner,
  children,
}: ControlsSheetLayoutProps) {
  const navigation = useNavigation();
  const isOwnerInStack = useNavigationState(sheet, (state) =>
    state.routes.some((route) => route.name === owner)
  );

  useEffect(() => {
    if (!isOwnerInStack) {
      navigation.goBack();
    }
  }, [isOwnerInStack, navigation]);

  return children;
}

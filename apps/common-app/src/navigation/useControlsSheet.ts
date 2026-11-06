import { useNavigation } from '@react-navigation/native';
import { useEffect } from 'react';

import type { DemoControlsKey, DemoKey } from '../demos';
import type { ExampleControlsKey, ExampleKey } from '../examples';

/** Route of an example's or demo's controls sheet. */
export type ControlsSheetKey = ExampleControlsKey | DemoControlsKey;
/** Screen a controls sheet belongs to. */
export type ControlsSheetOwner = ExampleKey | DemoKey;

/**
 * Presents the screen's controls sheet (registered by `RootStack` for every
 * example or demo with a `controls` component) for as long as the screen is
 * mounted. The sheet can't be swiped away; it leaves together with its screen.
 */
export function useControlsSheet(route: ControlsSheetKey) {
  const navigation = useNavigation();

  useEffect(() => {
    navigation.navigate(route);
  }, [navigation, route]);
}

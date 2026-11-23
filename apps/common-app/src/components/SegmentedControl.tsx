import SegmentedControlClass, {
  type SegmentedControlProps as NativeProps,
} from '@react-native-segmented-control/segmented-control';
import React, { ComponentType } from 'react';

// The package's class typing builds on `NativeMethods`, which React Native
// 0.88's types no longer satisfy; its props are still accurate.
const NativeSegmentedControl =
  SegmentedControlClass as unknown as ComponentType<NativeProps>;

export interface SegmentedControlOption<T extends string> {
  value: T;
  label: string;
}

interface SegmentedControlProps<T extends string> {
  options: ReadonlyArray<SegmentedControlOption<T>>;
  value: T;
  onChange: (value: T) => void;
  /** Fills the selected segment; the system colour when omitted. */
  color?: string;
}

/**
 * A row of 2–5 mutually exclusive options: `UISegmentedControl` on iOS, the
 * library's JS rendering of it on Android.
 */
function SegmentedControl<T extends string>({
  options,
  value,
  onChange,
  color,
}: SegmentedControlProps<T>) {
  return (
    <NativeSegmentedControl
      appearance="dark"
      values={options.map((option) => option.label)}
      selectedIndex={options.findIndex((option) => option.value === value)}
      onChange={(event) =>
        onChange(options[event.nativeEvent.selectedSegmentIndex].value)
      }
      tintColor={color}
    />
  );
}

export default SegmentedControl;

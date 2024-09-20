/* eslint-disable react-native/no-inline-styles */
import type { ReactNode } from 'react';
import { StyleProp, ViewStyle } from 'react-native';

import { SafeAreaView } from 'react-native-safe-area-context';

export function Container({
  children,
  style,
  centered = true,
}: {
  children: ReactNode;
  style?: StyleProp<ViewStyle>;
  centered?: boolean;
}) {
  return (
    <SafeAreaView
      style={[
        {
          flex: 1,
          padding: 8,
        },
        centered && { justifyContent: 'center', alignItems: 'center' },
        style,
      ]}
    >
      {children}
    </SafeAreaView>
  );
}

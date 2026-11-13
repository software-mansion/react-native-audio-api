import { useHeaderHeight } from '@react-navigation/elements';
import React, { PropsWithChildren } from 'react';
import { StyleProp, StyleSheet, ViewStyle } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';

import { colors, layout } from '../styles';

type ContainerProps = PropsWithChildren<{
  style?: StyleProp<ViewStyle>;
  centered?: boolean;
  disablePadding?: boolean;
}>;

const contentGapBelowHeader = 8;

/**
 * Screen root for stack screens. The native header is transparent, so content
 * starts below the measured header height instead of under it.
 */
const Container: React.FC<ContainerProps> = (props) => {
  const { children, style, centered, disablePadding } = props;
  const headerHeight = useHeaderHeight();

  return (
    <SafeAreaView
      edges={['bottom', 'left', 'right']}
      style={[
        styles.basic,
        { paddingTop: headerHeight + contentGapBelowHeader },
        centered && styles.centered,
        !disablePadding && styles.padding,
        style,
      ]}
    >
      {children}
    </SafeAreaView>
  );
};

export default Container;

const styles = StyleSheet.create({
  basic: {
    flex: 1,
    backgroundColor: colors.background,
  },
  padding: {
    paddingHorizontal: layout.screenPadding,
  },
  centered: {
    alignItems: 'center',
    justifyContent: 'center',
  },
});

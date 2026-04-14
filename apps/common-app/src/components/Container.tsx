import React, { PropsWithChildren } from 'react';
import { StyleProp, StyleSheet, ViewStyle } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';

import { colors } from '../styles';

type ContainerProps = PropsWithChildren<{
  style?: StyleProp<ViewStyle>;
  centered?: boolean;
  disablePadding?: boolean;
  headless?: boolean;
}>;

const headerPadding = 120; // eyeballed

const Container: React.FC<ContainerProps> = (props) => {
  const { children, style, centered, disablePadding, headless } = props;

  return (
    <SafeAreaView
      edges={
        headless
          ? ['bottom', 'left', 'right', 'top']
          : ['bottom', 'left', 'right']
      }
      style={[
        headless ? styles.basicHeadless : styles.basic,
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
    paddingTop: headerPadding,
    backgroundColor: colors.background,
  },
  basicHeadless: {
    flex: 1,
    backgroundColor: colors.background,
    paddingTop: 20,
  },
  padding: {
    paddingHorizontal: 18,
  },
  centered: {
    alignItems: 'center',
    justifyContent: 'center',
  },
});

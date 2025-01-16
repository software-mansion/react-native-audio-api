import React, { PropsWithChildren } from 'react';
import { SafeAreaView } from 'react-native-safe-area-context';
import { StyleProp, StyleSheet, ViewStyle } from 'react-native';

import BGGradient from './BGGradient';
import { colors } from '../styles';

type ContainerProps = PropsWithChildren<{
  style?: StyleProp<ViewStyle>;
  centered?: boolean;
}>;

const headerPadding = 120; // eyeballed

const Container: React.FC<ContainerProps> = (props) => {
  const { children, style, centered } = props;

  return (
    <SafeAreaView
      edges={['bottom', 'left', 'right']}
      style={[styles.basic, centered && styles.centered, style]}>
      <BGGradient />
      {children}
    </SafeAreaView>
  );
};

export default Container;

const styles = StyleSheet.create({
  basic: {
    flex: 1,
    padding: 24,
    paddingTop: headerPadding,
    backgroundColor: colors.background,
  },
  centered: {
    alignItems: 'center',
    justifyContent: 'center',
  },
});

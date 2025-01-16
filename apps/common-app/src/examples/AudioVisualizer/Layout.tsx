import React, { PropsWithChildren } from 'react';
import { View, StyleSheet } from 'react-native';

import { layout, colors } from '../../styles';

const Box: React.FC<PropsWithChildren> = ({ children }) => {
  return <View style={styles.container}>{children}</View>;
};

const LineVertical: React.FC = () => <View style={styles.vertical} />;

const LineHorizontal: React.FC = () => <View style={styles.horizontal} />;

const Row: React.FC<PropsWithChildren> = ({ children }) => {
  return <View style={styles.row}>{children}</View>;
};

const Layout = {
  Box,
  Row,
  LineVertical,
  LineHorizontal,
};

export default Layout;

const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: layout.spacing,
  },
  vertical: {
    width: 1,
    height: '100%',
    backgroundColor: colors.border,
  },
  horizontal: {
    height: 1,
    width: '100%',
    backgroundColor: colors.border,
  },
  row: {
    flex: 1,
    flexDirection: 'row',
  },
});

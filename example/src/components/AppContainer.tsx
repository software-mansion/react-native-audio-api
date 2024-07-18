import React from 'react';
import { StyleSheet, View } from 'react-native';

function AppContainer({ children }: { children: React.ReactNode }) {
  return <View style={styles.container}>{children}</View>;
}

export default AppContainer;

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
});

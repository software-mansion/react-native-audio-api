import { DarkTheme, NavigationContainer } from '@react-navigation/native';
import type { FC } from 'react';
import React from 'react';
import { StyleSheet } from 'react-native';
import { GestureHandlerRootView } from 'react-native-gesture-handler';

import RootStackNavigator from './navigation/RootStack';

const linking = {
  prefixes: ['audioapi-example://'],
  config: {
    screens: {
      RecordDemo: 'record',
    },
  },
};

const App: FC = () => (
  <GestureHandlerRootView style={styles.container}>
    <NavigationContainer linking={linking} theme={DarkTheme}>
      <RootStackNavigator />
    </NavigationContainer>
  </GestureHandlerRootView>
);

export default App;

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
});

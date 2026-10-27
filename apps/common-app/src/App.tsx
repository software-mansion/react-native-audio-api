import {
  DarkTheme,
  LinkingOptions,
  NavigationContainer,
} from '@react-navigation/native';
import type { FC } from 'react';
import React from 'react';
import { StyleSheet } from 'react-native';
import { GestureHandlerRootView } from 'react-native-gesture-handler';

import RootStackNavigator, {
  type RootStackParamList,
} from './navigation/RootStack';

const linking: LinkingOptions<RootStackParamList> = {
  prefixes: ['audioapi-example://'],
  config: {
    initialRouteName: 'MainTabs',
    screens: {
      RecordDemo: 'record',
      // audioapi-example://orchestra?url=ws://host:8765 — what the conductor
      // dashboard's QR code encodes.
      Orchestra: 'orchestra',
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

/**
 * @format
 */

import { AppRegistry, Platform } from 'react-native';
import App from './App';
import { name as appName } from './app.json';

process.env.EXPO_OS = Platform.OS;

AppRegistry.registerComponent(appName, () => App);

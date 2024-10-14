import { Dimensions } from 'react-native';

const { width } = Dimensions.get('screen');

export const layout = {
  spacing: 8,
  radius: 8,
  knobSize: 24,
  indicatorSize: 48,
  screenWidth: width,
} as const;

export const colors = {
  white: '#ffffff',
  main: '#428ce7',
  border: 'rgba(0,0,0,0.1)',
  black: '#000000',
  gray: '#d7d7d7',
} as const;

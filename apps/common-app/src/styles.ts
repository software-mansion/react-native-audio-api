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
  border: '#999',
  main: '#38ACDD',
  black: '#000000',
  gray: '#d7d7d7',

  background: '#222222',
} as const;

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
  main: '#38ACDD',
  black: '#000000',
  gray: '#d7d7d7',
  yellow: '#FFD61E',

  background: '#222222',
  backgroundDark: '#1f2020',
  backgroundLight: '#333333',

  separator: '#333333',
  modalBackdrop: '#00000040',
  border: '#999999',
} as const;

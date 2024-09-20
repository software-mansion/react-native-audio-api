import { Dimensions } from 'react-native';

const { width } = Dimensions.get('screen');

export const layout = {
  spacing: 8,
  radius: 8,
  knobSize: 24,
  indicatorSize: 48,
  avatarSize: 36,
  imageHeight: 260,
  screenWidth: width,
};

export const colors = {
  white: '#ffffff',
  darkblue: '#001a72',
  blue: '#007AFF',
  border: 'rgba(0,0,0,0.1)',
};

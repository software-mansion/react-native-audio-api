import { Dimensions } from 'react-native';

const { width } = Dimensions.get('screen');

export const layout = {
  spacing: 8,
  radius: 4,
  cardRadius: 22,
  screenPadding: 16,
  knobSize: 24,
  indicatorSize: 48,
  screenWidth: width,
} as const;

/**
 * Dark palette from the audiodocs theme (`packages/docs-theme/css/colors.css`).
 * The named hues (`blue`, `green`, …) are its `-dark-100` ramp steps.
 */
export const colors = {
  white: '#ffffff',
  main: '#38ACDD',
  black: '#000000',
  gray: '#d7d7d7',
  yellow: '#FFD61E',
  accent: '#ff7774',
  onAccent: '#232736',

  blue: '#00a9f0',
  green: '#3fc684',
  purple: '#b07eff',
  navy: '#6676aa',

  background: '#232736',
  backgroundDark: '#1f2020',
  backgroundLight: '#32384d',
  surface: '#30354a',
  surfacePressed: '#3a4058',
  /** Sunken wells inside a screen, e.g. the recorder's waveform lane. */
  recess: '#272b3c',

  textPrimary: '#eef0ff',
  textSecondary: '#c1c6e5',
  textMuted: '#919fcf',

  track: 'rgba(238, 240, 255, 0.1)',
  separator: '#333333',
  listSeparator: 'rgba(238, 240, 255, 0.1)',
  border: '#999999',
} as const;

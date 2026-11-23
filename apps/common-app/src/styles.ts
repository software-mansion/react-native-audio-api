import { Dimensions, TextStyle } from 'react-native';

const { width } = Dimensions.get('screen');

export const layout = {
  spacing: 8,
  cardRadius: 22,
  screenPadding: 16,
  screenWidth: width,
} as const;

/**
 * Dark palette from the audiodocs theme (`packages/docs-theme/css/colors.css`).
 * The named hues (`blue`, `green`, …) are its `-dark-100` ramp steps.
 */
export const colors = {
  yellow: '#FFD61E',
  accent: '#ff7774',
  onAccent: '#232736',

  blue: '#00a9f0',
  green: '#3fc684',
  purple: '#b07eff',
  danger: '#e5484d',
  navy: '#6676aa',

  background: '#232736',
  surface: '#30354a',
  surfacePressed: '#3a4058',
  /** Sunken wells inside a screen, e.g. the recorder's waveform lane. */
  recess: '#272b3c',

  textPrimary: '#eef0ff',
  textSecondary: '#c1c6e5',
  textMuted: '#919fcf',

  track: 'rgba(238, 240, 255, 0.1)',
} as const;

export const typography = {
  caption: {
    color: colors.textMuted,
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 1.2,
    textTransform: 'uppercase',
  },
  hint: {
    color: colors.textSecondary,
    fontSize: 13,
    lineHeight: 18,
  },
  sectionTitle: {
    color: colors.textPrimary,
    fontSize: 17,
    fontWeight: '600',
  },
} satisfies Record<string, TextStyle>;

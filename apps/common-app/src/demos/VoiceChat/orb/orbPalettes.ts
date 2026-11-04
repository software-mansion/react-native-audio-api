/** What the orb shows; derived from the session status, not from audio. */
export type OrbState = 'idle' | 'listening' | 'speaking' | 'paused' | 'error';

export type OrbColor = [number, number, number];

export type OrbPalette = {
  primary: OrbColor;
  accent: OrbColor;
  highlight: OrbColor;
};

function rgb(hex: string): OrbColor {
  return [
    Number.parseInt(hex.slice(1, 3), 16) / 255,
    Number.parseInt(hex.slice(3, 5), 16) / 255,
    Number.parseInt(hex.slice(5, 7), 16) / 255,
  ];
}

function palette(primary: string, accent: string, highlight: string) {
  return {
    primary: rgb(primary),
    accent: rgb(accent),
    highlight: rgb(highlight),
  };
}

/** The aec-workshop orb's dark-appearance palettes; the app is dark-only. */
export const orbPalettes: Record<OrbState, OrbPalette> = {
  idle: palette('#68717F', '#8893A2', '#C7D0DB'),
  listening: palette('#46B5C3', '#6A9FD8', '#B6E9D5'),
  speaking: palette('#A58AD9', '#D197C3', '#E4D1F3'),
  paused: palette('#7F94AD', '#A2B2C7', '#D0DBE8'),
  error: palette('#D66F80', '#E79AA5', '#F2C5C1'),
};

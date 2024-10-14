export type KeyName =
  | 'C4'
  | 'C#4'
  | 'D4'
  | 'D#4'
  | 'E4'
  | 'F4'
  | 'F#4'
  | 'G4'
  | 'G#4'
  | 'A4'
  | 'A#4'
  | 'B4';

export interface Key {
  name: KeyName;
  frequency: number;
}

export type KeyMap = Record<KeyName, Key>;

export const keyMap: KeyMap = {
  'C4': { name: 'C4', frequency: 261.626 },
  'C#4': { name: 'C#4', frequency: 277.183 },
  'D4': { name: 'D4', frequency: 293.665 },
  'D#4': { name: 'D#4', frequency: 311.127 },
  'E4': { name: 'E4', frequency: 329.628 },
  'F4': { name: 'F4', frequency: 349.228 },
  'F#4': { name: 'F#4', frequency: 369.994 },
  'G4': { name: 'G4', frequency: 391.995 },
  'G#4': { name: 'G#4', frequency: 415.305 },
  'A4': { name: 'A4', frequency: 440.0 },
  'A#4': { name: 'A#4', frequency: 466.164 },
  'B4': { name: 'B4', frequency: 493.883 },
} as const;

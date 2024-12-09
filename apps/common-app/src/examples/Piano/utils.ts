import { AudioBuffer } from 'react-native-audio-api';

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

export const sourcesTone: Partial<Record<KeyName, string>> = {
  'C4': 'https://tonejs.github.io/audio/salamander/C4.mp3',
  'D#4': 'https://tonejs.github.io/audio/salamander/Ds4.mp3',
  'F#4': 'https://tonejs.github.io/audio/salamander/Fs4.mp3',
  'A4': 'https://tonejs.github.io/audio/salamander/A4.mp3',
};

export const sourcesTeda: Partial<Record<KeyName, string>> = {
  'C4': 'https://michalsek.github.io/audio-samples/piano/1_C4.mp3',
  'C#4': 'https://michalsek.github.io/audio-samples/piano/2_Ch4.mp3',
  'D4': 'https://michalsek.github.io/audio-samples/piano/3_D4.mp3',
  'D#4': 'https://michalsek.github.io/audio-samples/piano/4_Dh4.mp3',
  'E4': 'https://michalsek.github.io/audio-samples/piano/5_E4.mp3',
  'F4': 'https://michalsek.github.io/audio-samples/piano/6_F4.mp3',
  'F#4': 'https://michalsek.github.io/audio-samples/piano/7_Fh4.mp3',
  'G4': 'https://michalsek.github.io/audio-samples/piano/8_G4.mp3',
  'G#4': 'https://michalsek.github.io/audio-samples/piano/9_Gh4.mp3',
  'A4': 'https://michalsek.github.io/audio-samples/piano/10_A4.mp3',
  'A#4': 'https://michalsek.github.io/audio-samples/piano/11_Ah4.mp3',
  'B4': 'https://michalsek.github.io/audio-samples/piano/12_B4.mp3',
};

export const sources = sourcesTeda;

export const keyMap: KeyMap = {
  'C4': {
    name: 'C4',
    frequency: 261.626,
  },
  'C#4': {
    name: 'C#4',
    frequency: 277.183,
  },
  'D4': {
    name: 'D4',
    frequency: 293.665,
  },
  'D#4': {
    name: 'D#4',
    frequency: 311.127,
  },
  'E4': {
    name: 'E4',
    frequency: 329.628,
  },
  'F4': {
    name: 'F4',
    frequency: 349.228,
  },
  'F#4': {
    name: 'F#4',
    frequency: 369.994,
  },
  'G4': {
    name: 'G4',
    frequency: 391.995,
  },
  'G#4': {
    name: 'G#4',
    frequency: 415.305,
  },
  'A4': {
    name: 'A4',
    frequency: 440.0,
  },
  'A#4': {
    name: 'A#4',
    frequency: 466.164,
  },
  'B4': {
    name: 'B4',
    frequency: 493.883,
  },
} as const;

export function getClosest(key: Key) {
  let closestKey = keyMap.C4;
  let minDiff = closestKey.frequency - key.frequency;

  for (const sourcedKeys of Object.keys(sources)) {
    const currentKey = keyMap[sourcedKeys as KeyName];

    const diff = currentKey.frequency - key.frequency;

    if (Math.abs(diff) < Math.abs(minDiff)) {
      minDiff = diff;
      closestKey = currentKey;
    }
  }

  return closestKey;
}

export function getSource(bufferList: Record<KeyName, AudioBuffer>, key: Key) {
  if (key.name in bufferList) {
    return { buffer: bufferList[key.name], playbackRate: 1 };
  }

  const closestKey = getClosest(key);

  return {
    buffer: bufferList[closestKey.name],
    playbackRate: key.frequency / closestKey.frequency,
  };
}

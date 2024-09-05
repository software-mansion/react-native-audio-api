import { AudioContext } from 'react-native-audio-api';

export interface SoundEngine {
  audioContext: AudioContext;
  tone: number;
  decay: number;
  volume: number;
  play: (time: number) => void;
}

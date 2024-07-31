import { AudioContext } from 'react-native-audio-context';

export interface SoundEngine {
  audioContext: AudioContext;
  tone: number;
  decay: number;
  volume: number;
  play: (time: number) => void;
}

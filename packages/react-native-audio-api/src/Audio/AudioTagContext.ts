import { createContext, useContext } from 'react';
import type { AudioTagPlaybackState, PreloadType } from './types';

export type AudioComponentContextType = {
  play: () => void;
  pause: () => void;
  seekToTime: (seconds: number) => void;
  setVolume: (volume: number) => void;
  setMuted: (muted: boolean) => void;

  ready: boolean;
  volume: number;
  muted: boolean;
  playbackState: AudioTagPlaybackState;
  currentTime: number;
  duration: number;
  autoPlay: boolean;
  loop: boolean;
  preload: PreloadType;
  playbackRate: number;
  preservesPitch: boolean;
};

export const AudioComponentContext = createContext<
  AudioComponentContextType | undefined
>(undefined);

export function useAudioTagContext(): AudioComponentContextType {
  const context = useContext(AudioComponentContext);

  if (context === undefined) {
    throw new Error(
      'useAudioTagContext must be used within an <Audio> component.'
    );
  }

  return context;
}

import { createContext, useContext } from 'react';
import type { AudioTagPlaybackState } from './types';

export type AudioComponentContextType = {
  play: () => void;
  pause: () => void;
  seekToTime: (seconds: number) => void;
  volume: number;
  setVolume: (volume: number) => void;
  muted: boolean;
  setMuted: (muted: boolean) => void;
  isReady: boolean;
  playbackState: AudioTagPlaybackState;
  currentTime: number;
  duration: number;
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

import type { SessionOptions, LockScreenInfo } from './types';

export interface IAudioManager {
  setOptions: (options: SessionOptions) => void;
  setNowPlaying: (info: LockScreenInfo) => void;
}

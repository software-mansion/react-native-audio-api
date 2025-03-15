import type { SessionOptions } from './types';

export interface IAudioManager {
  setOptions: (options: SessionOptions) => void;
}

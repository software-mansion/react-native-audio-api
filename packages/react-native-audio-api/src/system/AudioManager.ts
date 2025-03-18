import type { IAudioManager } from './interface';
import { SessionOptions, LockScreenInfo } from './types';

class AudioManager {
  protected module(): IAudioManager {
    return global.AudioManager;
  }

  setOptions(options: SessionOptions) {
    this.module().setOptions(options);
  }

  setNowPlaying(info: LockScreenInfo) {
    this.module().setNowPlaying(info);
  }
}

export default new AudioManager();

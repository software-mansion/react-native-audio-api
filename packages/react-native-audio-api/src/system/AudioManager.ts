import type { IAudioManager } from './interface';
import { SessionOptions } from './types';

class AudioManager {
  protected module(): IAudioManager {
    return global.AudioManager;
  }

  setOptions(options: SessionOptions) {
    this.module().setOptions(options);
  }
}

export default new AudioManager();

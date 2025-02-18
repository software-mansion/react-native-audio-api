import { IAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import { AudioContextOptions } from '../types';

export default class AudioContext extends BaseAudioContext {
  constructor(options?: AudioContextOptions) {
    super(global.__AudioAPIInstaller.createAudioContext(options?.sampleRate));
  }

  close(): void {
    (this.context as IAudioContext).close();
  }
}

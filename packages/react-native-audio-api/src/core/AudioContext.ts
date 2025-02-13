import { IAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';

export default class AudioContext extends BaseAudioContext {
  constructor(sampleRate?: number) {
    super(global.__AudioAPIInstaller.createAudioContext(sampleRate));
  }

  close(): void {
    (this.context as IAudioContext).close();
  }
}

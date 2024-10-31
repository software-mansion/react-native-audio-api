import { IAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';

export default class AudioContext extends BaseAudioContext {
  constructor() {
    super(global.__AudioAPIInstaller.createAudioContext());
  }

  close(): void {
    (this.context as IAudioContext).close();
  }
}

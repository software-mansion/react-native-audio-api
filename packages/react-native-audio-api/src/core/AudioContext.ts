import AudioAPIModule from '../AudioAPIModule';
import { assertSupportedSampleRate } from '../utils/audioConstants';
import { AudioTagHandle } from '../Audio/types';
import { IAudioContext } from '../jsi-interfaces';
import AudioManager from '../system';
import { AudioContextOptions } from '../types';
import BaseAudioContext from './BaseAudioContext';
import MediaElementAudioSourceNode from './MediaElementAudioSourceNode';

export default class AudioContext extends BaseAudioContext {
  constructor(options?: AudioContextOptions) {
    if (options?.sampleRate != null) {
      assertSupportedSampleRate(options.sampleRate);
    }

    const audioRuntime = AudioAPIModule.createAudioRuntime();

    super(
      globalThis.createAudioContext(
        options?.sampleRate || AudioManager.getDevicePreferredSampleRate(),
        audioRuntime
      )
    );
  }

  async close(): Promise<void> {
    return (this.context as IAudioContext).close();
  }

  async resume(): Promise<void> {
    await (this.context as IAudioContext).resume();
  }

  async suspend(): Promise<void> {
    await (this.context as IAudioContext).suspend();
  }

  createMediaElementSource(
    mediaElement: AudioTagHandle
  ): MediaElementAudioSourceNode {
    return new MediaElementAudioSourceNode(this, { mediaElement });
  }
}

import { InvalidStateError } from '../errors';
import { assertSupportedSampleRate } from '../utils/validation';
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

    super(
      globalThis.createAudioContext(
        options?.sampleRate || AudioManager.getDevicePreferredSampleRate()
      )
    );
  }

  public get baseLatency(): number {
    return (this.context as IAudioContext).baseLatency;
  }

  public get outputLatency(): number {
    return (this.context as IAudioContext).outputLatency;
  }

  async close(): Promise<undefined> {
    if (this.contextState === 'closed') {
      throw new InvalidStateError('Cannot close a closed audio context.');
    }

    this.contextState = 'closed';
    return (this.context as IAudioContext).close();
  }

  async resume(): Promise<undefined> {
    if (this.contextState === 'closed') {
      throw new InvalidStateError('Cannot resume a closed audio context.');
    }

    this.contextState = 'running';
    return (this.context as IAudioContext).resume();
  }

  async suspend(): Promise<undefined> {
    if (this.contextState === 'closed') {
      throw new InvalidStateError('Cannot suspend a closed audio context.');
    }

    this.contextState = 'suspended';
    return (this.context as IAudioContext).suspend();
  }

  createMediaElementSource(
    mediaElement: AudioTagHandle
  ): MediaElementAudioSourceNode {
    return new MediaElementAudioSourceNode(this, { mediaElement });
  }
}

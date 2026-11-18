import { assertSupportedSampleRate } from '../utils/validation';
import { AudioContextOptions } from '../types';
import BaseAudioContext from './BaseAudioContext.web';
import MediaElementAudioSourceNode from './MediaElementAudioSourceNode.web';

export default class AudioContext extends BaseAudioContext<globalThis.AudioContext> {
  public onerror: (() => void) | null = null;

  constructor(options?: AudioContextOptions) {
    if (options?.sampleRate != null) {
      assertSupportedSampleRate(options.sampleRate);
    }

    super(
      new window.AudioContext({
        sampleRate: options?.sampleRate,
        latencyHint: options?.latencyHint,
      })
    );

    this.context.addEventListener('error', () => this.onerror?.());
  }

  public get baseLatency(): number {
    return this.context.baseLatency;
  }

  public get outputLatency(): number {
    return this.context.outputLatency;
  }

  createMediaElementSource(
    mediaElement: HTMLMediaElement
  ): MediaElementAudioSourceNode {
    return new MediaElementAudioSourceNode(
      this,
      this.context.createMediaElementSource(mediaElement),
      mediaElement
    );
  }

  async close(): Promise<void> {
    await this.context.close();
  }

  async resume(): Promise<void> {
    await this.context.resume();
  }

  async suspend(): Promise<void> {
    await this.context.suspend();
  }
}

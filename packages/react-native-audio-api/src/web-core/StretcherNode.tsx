import BaseAudioContext from './BaseAudioContext';
import AudioBuffer from './AudioBuffer';

import { globalWasmPromise, globalTag } from './custom/LoadCustomWasm';

declare class IStretcherNode {
  constructor(context: globalThis.BaseAudioContext);

  addBuffers(channels: number[][]): void;
  schedule(params: { rate: number }): void;
  start(): void;
  stop(): void;
}

declare global {
  interface Window {
    [globalTag]: typeof IStretcherNode;
  }
}

export default class StretcherNode {
  stretcher: IStretcherNode | null = null;
  internalBuffer: AudioBuffer | null = null;
  internalPlaybackRate: number = 1;
  isPlaying = false;

  constructor(context: BaseAudioContext) {
    this.internalBuffer = null;

    globalWasmPromise?.then(() => {
      this.stretcher = new window[globalTag](context.context);
    });
  }

  public get buffer(): AudioBuffer | null {
    return this.internalBuffer;
  }

  public set buffer(buffer: AudioBuffer) {
    this.internalBuffer = buffer;
    const channelArrays = [];

    for (let i = 0; i < buffer.numberOfChannels; i++) {
      channelArrays.push(buffer.getChannelData(i));
    }

    this.stretcher?.addBuffers(channelArrays);
  }

  public get playbackRate(): number {
    return this.internalPlaybackRate;
  }

  public set playbackRate(value: number) {
    this.internalPlaybackRate = value;

    if (this.isPlaying) {
      this.stretcher?.schedule({ rate: value });
    }
  }

  public start(when?: number, offset?: number, duration?: number): void {
    this.isPlaying = true;

    this.stretcher?.schedule({ rate: this.internalPlaybackRate });
    this.stretcher?.start();
  }

  public stop(when?: number): void {
    this.isPlaying = false;
    this.stretcher?.stop();
  }
}

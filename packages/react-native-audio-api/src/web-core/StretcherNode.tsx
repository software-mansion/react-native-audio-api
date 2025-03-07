import BaseAudioContext from './BaseAudioContext';
import AudioBuffer from './AudioBuffer';
import AudioNode from './AudioNode';

import { globalTag } from './custom/LoadCustomWasm';

interface ScheduleOptions {
  rate?: number;
  active?: boolean;
  outputTime?: number;
  semitones?: number;
  loopStart?: number;
  loopEnd?: number;
}

declare class IStretcherNode {
  addBuffers(channels: Float32Array[]): void;

  schedule(options: ScheduleOptions): void;

  start(
    when?: number,
    offset?: number,
    duration?: number,
    rate?: number,
    semitones?: number
  ): void;

  stop(when?: number): void;

  connect(destination: globalThis.AudioNode): void;
  disconnect(destination: globalThis.AudioNode): void;
}

declare global {
  interface Window {
    [globalTag]: (
      audioContext: globalThis.BaseAudioContext
    ) => Promise<IStretcherNode>;
  }
}

export default class StretcherNode extends AudioNode {
  _buffer: AudioBuffer | null = null;

  _playbackRate: number = 1;
  _loopStart: number = -1;
  _loopEnd: number = -1;
  _loop = false;
  _isPlaying = false;

  constructor(context: BaseAudioContext, node: IStretcherNode) {
    super(context, node as unknown as globalThis.AudioNode);
    this._buffer = null;
  }

  public get buffer(): AudioBuffer | null {
    return this._buffer;
  }

  public set buffer(buffer: AudioBuffer) {
    this._buffer = buffer;
    const channelArrays: Float32Array[] = [];

    for (let i = 0; i < buffer.numberOfChannels; i += 1) {
      channelArrays.push(buffer.getChannelData(i));
    }

    (this.node as unknown as IStretcherNode).addBuffers(channelArrays);
  }

  public get playbackRate(): number {
    return this._playbackRate;
  }

  public set playbackRate(value: number) {
    this._playbackRate = value;

    if (this._isPlaying) {
      (this.node as unknown as IStretcherNode).schedule({ rate: value });
    }
  }

  public get loopStart(): number {
    return this._loopStart;
  }

  public set loopStart(value: number) {
    this._loopStart = value;

    if (this._isPlaying) {
      (this.node as unknown as IStretcherNode).schedule({ loopStart: value });
    }
  }

  public get loopEnd(): number {
    return this._loopEnd;
  }

  public set loopEnd(value: number) {
    this._loopEnd = value;

    if (this._isPlaying) {
      (this.node as unknown as IStretcherNode).schedule({ loopEnd: value });
    }
  }

  public get loop(): boolean {
    return this._loop;
  }

  public set loop(value: boolean) {
    this._loop = value;
  }

  public start(
    when?: number,
    offset?: number,
    duration?: number,
    rate?: number,
    semitones?: number
  ): void {
    this._isPlaying = true;

    if (this.loop && this._loopStart !== -1 && this._loopEnd !== -1) {
      (this.node as unknown as IStretcherNode).schedule({
        loopStart: this._loopStart,
        loopEnd: this._loopEnd,
      });
    }

    (this.node as unknown as IStretcherNode).start(
      when ?? 0,
      offset ?? 0,
      duration ?? this._buffer?.duration ?? 0,
      rate ?? this._playbackRate ?? 0,
      semitones ?? 0
    );
  }

  public stop(when?: number): void {
    this._isPlaying = false;
    (this.node as unknown as IStretcherNode).stop(when);
  }
}

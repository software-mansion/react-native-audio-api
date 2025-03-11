import BaseAudioContext from './BaseAudioContext';
import AudioBuffer from './AudioBuffer';
import AudioNode from './AudioNode';

import { clamp } from '../utils';
import { globalTag } from './custom/LoadCustomWasm';

interface ScheduleOptions {
  rate?: number;
  active?: boolean;
  output?: number;
  input?: number;
  semitones?: number;
  loopStart?: number;
  loopEnd?: number;
}

declare class IStretcherNode {
  addBuffers(channels: Float32Array[]): void;
  dropBuffers(): void;

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

class StretcherNodeAudioParam {
  private _value: number;
  private _setter: (value: number, when?: number) => void;

  constructor(value: number, setter: (value: number, when?: number) => void) {
    this._value = value;
    this._setter = setter;
  }

  public get value(): number {
    return this._value;
  }

  public set value(value: number) {
    this._value = value;

    this._setter(value);
  }

  public setValueAtTime(value: number, startTime: number): void {
    this._value = value;

    this._setter(value, startTime);
  }
}

export default class StretcherNode extends AudioNode {
  readonly playbackRate: StretcherNodeAudioParam;
  readonly detune: StretcherNodeAudioParam;

  private _loop = false;
  private _loopEnd: number = -1;
  private _loopStart: number = -1;
  private _isPlaying = false;

  private _buffer: AudioBuffer | null = null;

  private asStretcher(): IStretcherNode {
    return this.node as unknown as IStretcherNode;
  }

  constructor(context: BaseAudioContext, node: IStretcherNode) {
    super(context, node as unknown as globalThis.AudioNode);

    this.playbackRate = new StretcherNodeAudioParam(
      1,
      this.setPlaybackRate.bind(this)
    );

    this.detune = new StretcherNodeAudioParam(0, this.setDetune.bind(this));
  }

  public get isPlaying(): boolean {
    return this._isPlaying;
  }

  public setPlaybackRate(value: number, when?: number): void {
    if (this._isPlaying) {
      this.asStretcher().schedule({
        rate: value,
        output: when,
      });
    }
  }

  public setDetune(value: number, when?: number): void {
    if (this._isPlaying) {
      this.asStretcher().schedule({
        semitones: Math.floor(Math.max(Math.min(value / 100, 12), -12)),
        output: when,
      });
    }
  }

  public get buffer(): AudioBuffer | null {
    return this._buffer;
  }

  public set buffer(buffer: AudioBuffer) {
    this._buffer = buffer;

    const stretcher = this.asStretcher();

    stretcher.dropBuffers();

    const channelArrays: Float32Array[] = [];

    for (let i = 0; i < buffer.numberOfChannels; i += 1) {
      channelArrays.push(buffer.getChannelData(i));
    }

    stretcher.addBuffers(channelArrays);
  }

  public get loop(): boolean {
    return this._loop;
  }

  public set loop(value: boolean) {
    this._loop = value;
  }

  public get loopStart(): number {
    return this._loopStart;
  }

  public set loopStart(value: number) {
    this._loopStart = value;
  }

  public get loopEnd(): number {
    return this._loopEnd;
  }

  public set loopEnd(value: number) {
    this._loopEnd = value;
  }

  public start(when?: number, offset?: number, duration?: number): void {
    this._isPlaying = true;
    const startAt = when ?? this.context.currentTime;
    const offsetBy = offset
      ? startAt +
        clamp(offset, 0, duration ?? this._buffer?.duration ?? Infinity)
      : undefined;

    if (this.loop && this._loopStart !== -1 && this._loopEnd !== -1) {
      this.asStretcher().schedule({
        loopStart: this._loopStart,
        loopEnd: this._loopEnd,
      });
    }

    this.asStretcher().start(
      startAt,
      offsetBy,
      duration,
      this.playbackRate.value,
      Math.floor(clamp(this.detune.value / 100, -12, 12))
    );
  }

  public stop(when?: number): void {
    this.asStretcher().stop(when);
  }
}

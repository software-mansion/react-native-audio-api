import { InvalidStateError, RangeError } from '../errors';

import AudioParam from './AudioParam';
import AudioBuffer from './AudioBuffer';
import BaseAudioContext from './BaseAudioContext';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';

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

interface IStretcherNode extends globalThis.AudioNode {
  channelCount: number;
  channelCountMode: globalThis.ChannelCountMode;
  channelInterpretation: globalThis.ChannelInterpretation;
  context: globalThis.BaseAudioContext;
  numberOfInputs: number;
  numberOfOutputs: number;

  onEnded:
    | ((this: globalThis.AudioScheduledSourceNode, ev: Event) => unknown)
    | null;
  addEventListener: (
    type: string,
    listener: EventListenerOrEventListenerObject | null,
    options?: boolean | AddEventListenerOptions
  ) => void;
  dispatchEvent: (event: Event) => boolean;
  removeEventListener: (
    type: string,
    callback: EventListenerOrEventListenerObject | null,
    options?: boolean | EventListenerOptions
  ) => void;

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

  connect(
    destination: globalThis.AudioNode,
    output?: number,
    input?: number
  ): globalThis.AudioNode;
  connect(destination: globalThis.AudioParam, output?: number): void;

  disconnect(): void;
  disconnect(output: number): void;

  disconnect(destination: globalThis.AudioNode): globalThis.AudioNode;
  disconnect(destination: globalThis.AudioNode, output: number): void;
  disconnect(
    destination: globalThis.AudioNode,
    output: number,
    input: number
  ): void;

  disconnect(destination: globalThis.AudioParam): void;
  disconnect(destination: globalThis.AudioParam, output: number): void;
}

class IStretcherNodeAudioParam implements globalThis.AudioParam {
  private _value: number;
  private _setter: (value: number, when?: number) => void;

  public automationRate: AutomationRate;
  public defaultValue: number;
  public maxValue: number;
  public minValue: number;

  constructor(
    value: number,
    setter: (value: number, when?: number) => void,
    automationRate: AutomationRate,
    minValue: number,
    maxValue: number,
    defaultValue: number
  ) {
    this._value = value;
    this.automationRate = automationRate;
    this.minValue = minValue;
    this.maxValue = maxValue;
    this.defaultValue = defaultValue;
    this._setter = setter;
  }

  public get value(): number {
    return this._value;
  }

  public set value(value: number) {
    this._value = value;

    this._setter(value);
  }

  cancelAndHoldAtTime(cancelTime: number): globalThis.AudioParam {
    this._setter(this.defaultValue, cancelTime);
    return this;
  }

  cancelScheduledValues(cancelTime: number): globalThis.AudioParam {
    this._setter(this.defaultValue, cancelTime);
    return this;
  }

  exponentialRampToValueAtTime(
    _value: number,
    _endTime: number
  ): globalThis.AudioParam {
    console.warn(
      'exponentialRampToValueAtTime is not implemented for pitch correction mode'
    );
    return this;
  }

  linearRampToValueAtTime(
    _value: number,
    _endTime: number
  ): globalThis.AudioParam {
    console.warn(
      'linearRampToValueAtTime is not implemented for pitch correction mode'
    );
    return this;
  }

  setTargetAtTime(
    _target: number,
    _startTime: number,
    _timeConstant: number
  ): globalThis.AudioParam {
    console.warn(
      'setTargetAtTime is not implemented for pitch correction mode'
    );
    return this;
  }

  setValueAtTime(value: number, startTime: number): globalThis.AudioParam {
    this._setter(value, startTime);
    return this;
  }

  setValueCurveAtTime(
    _values: Float32Array,
    _startTime: number,
    _duration: number
  ): globalThis.AudioParam {
    console.warn(
      'setValueCurveAtTime is not implemented for pitch correction mode'
    );
    return this;
  }
}

type DefaultSource = globalThis.AudioBufferSourceNode;

type IAudioBufferSourceNode = DefaultSource | IStretcherNode;

declare global {
  interface Window {
    [globalTag]: (
      audioContext: globalThis.BaseAudioContext
    ) => Promise<IStretcherNode>;
  }
}

export default class AudioBufferSourceNode<
  T extends IAudioBufferSourceNode = DefaultSource,
> extends AudioScheduledSourceNode {
  private _pitchCorrection: boolean;
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  private _loop: boolean = false;
  private _loopStart: number = -1;
  private _loopEnd: number = -1;

  private _buffer: AudioBuffer | null = null;

  constructor(context: BaseAudioContext, node: T, pitchCorrection: boolean) {
    super(context, node);

    this._pitchCorrection = pitchCorrection;

    if (pitchCorrection) {
      this.detune = new AudioParam(
        new IStretcherNodeAudioParam(
          0,
          this.setDetune.bind(this),
          'a-rate',
          -1200,
          1200,
          0
        ),
        context
      );

      this.playbackRate = new AudioParam(
        new IStretcherNodeAudioParam(
          1,
          this.setPlaybackRate.bind(this),
          'a-rate',
          0,
          Infinity,
          1
        ),
        context
      );
    } else {
      this.detune = new AudioParam((node as DefaultSource).detune, context);
      this.playbackRate = new AudioParam(
        (node as DefaultSource).playbackRate,
        context
      );
    }
  }

  private isStretcherNode() {
    return this._pitchCorrection;
  }

  private asStretcher(): IStretcherNode {
    return this.node as IStretcherNode;
  }

  private asBufferSource(): DefaultSource {
    return this.node as DefaultSource;
  }

  public setDetune(value: number, when: number = 0): void {
    if (!this.isStretcherNode() || !this.hasBeenStarted) {
      return;
    }

    this.asStretcher().schedule({
      semitones: Math.floor(clamp(value / 100, -12, 12)),
      output: when,
    });
  }

  public setPlaybackRate(value: number, when: number = 0): void {
    if (!this.isStretcherNode() || !this.hasBeenStarted) {
      return;
    }

    this.asStretcher().schedule({
      rate: value,
      output: when,
    });
  }

  public get buffer(): AudioBuffer | null {
    if (this.isStretcherNode()) {
      return this._buffer;
    }

    const buffer = this.asBufferSource().buffer;

    if (!buffer) {
      return null;
    }

    return new AudioBuffer(buffer);
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (this.isStretcherNode()) {
      this._buffer = buffer;

      const stretcher = this.asStretcher();
      stretcher.dropBuffers();

      if (!buffer) {
        return;
      }

      const channelArrays: Float32Array[] = [];

      for (let i = 0; i < buffer.numberOfChannels; i++) {
        channelArrays.push(buffer.getChannelData(i));
      }

      stretcher.addBuffers(channelArrays);
      return;
    }

    if (!buffer) {
      this.asBufferSource().buffer = null;
      return;
    }

    this.asBufferSource().buffer = buffer.buffer;
  }

  public get loop(): boolean {
    if (this.isStretcherNode()) {
      return this._loop;
    }

    return this.asBufferSource().loop;
  }

  public set loop(value: boolean) {
    if (this.isStretcherNode()) {
      this._loop = value;
      return;
    }

    this.asBufferSource().loop = value;
  }

  public get loopStart(): number {
    if (this.isStretcherNode()) {
      return this._loopStart;
    }

    return this.asBufferSource().loopStart;
  }

  public set loopStart(value: number) {
    if (this.isStretcherNode()) {
      this._loopStart = value;
      return;
    }

    this.asBufferSource().loopStart = value;
  }

  public get loopEnd(): number {
    if (this.isStretcherNode()) {
      return this._loopEnd;
    }

    return this.asBufferSource().loopEnd;
  }

  public set loopEnd(value: number) {
    if (this.isStretcherNode()) {
      this._loopEnd = value;
      return;
    }

    this.asBufferSource().loopEnd = value;
  }

  public start(when?: number, offset?: number, duration?: number): void {
    if (when && when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (offset && offset < 0) {
      throw new RangeError(
        `offset must be a finite non-negative number: ${offset}`
      );
    }

    if (duration && duration < 0) {
      throw new RangeError(
        `duration must be a finite non-negative number: ${duration}`
      );
    }

    if (this.hasBeenStarted) {
      throw new InvalidStateError('Cannot call start more than once');
    }

    this.hasBeenStarted = true;

    if (!this.isStretcherNode()) {
      this.asBufferSource().start(when, offset, duration);
      return;
    }

    const startAt =
      !when || when < this.context.currentTime
        ? this.context.currentTime
        : when;

    if (this.loop && this._loopStart !== -1 && this._loopEnd !== -1) {
      this.asStretcher().schedule({
        loopStart: this._loopStart,
        loopEnd: this._loopEnd,
      });
    }

    this.asStretcher().start(
      startAt,
      offset,
      duration,
      this.playbackRate.value,
      Math.floor(clamp(this.detune.value / 100, -12, 12))
    );
  }

  public stop(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (!this.hasBeenStarted) {
      throw new InvalidStateError(
        'Cannot call stop without calling start first'
      );
    }

    if (!this.isStretcherNode()) {
      this.asBufferSource().stop(when);
      return;
    }

    this.asStretcher().stop(when);
  }
}

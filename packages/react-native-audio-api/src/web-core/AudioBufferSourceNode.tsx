import { InvalidStateError, RangeError } from '../errors';

import AudioParam from './AudioParam';
import AudioBuffer from './AudioBuffer';
import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';

import { clamp } from '../utils';
import { AudioBufferSourceOptions } from '../types';
import { globalWasmPromise, globalTag } from './custom/LoadCustomWasm';

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

declare global {
  interface Window {
    [globalTag]: (
      audioContext: globalThis.BaseAudioContext
    ) => Promise<IStretcherNode>;
  }
}

interface IAudioAPIBufferSourceNodeWeb {
  connect(destination: AudioNode | AudioParam): AudioNode | AudioParam;
  disconnect(destination?: AudioNode | AudioParam): void;
  start(when?: number, offset?: number, duration?: number): void;
  stop(when: number): void;
  setDetune(value: number, when?: number): void;
  setPlaybackRate(value: number, when?: number): void;
  get buffer(): AudioBuffer | null;
  set buffer(buffer: AudioBuffer | null);
  get loop(): boolean;
  set loop(value: boolean);
  get loopStart(): number;
  set loopStart(value: number);
  get loopEnd(): number;
  set loopEnd(value: number);
}

class AudioBufferSourceNodeStretcher implements IAudioAPIBufferSourceNodeWeb {
  private stretcherPromise: Promise<IStretcherNode> | null = null;
  private node: IStretcherNode | null = null;
  private hasBeenStarted: boolean = false;
  private context: BaseAudioContext;
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  private _loop: boolean = false;
  private _loopStart: number = -1;
  private _loopEnd: number = -1;

  private _buffer: AudioBuffer | null = null;

  constructor(context: BaseAudioContext) {
    const promise = async () => {
      await globalWasmPromise;
      return window[globalTag](new window.AudioContext());
    };
    this.context = context;
    this.stretcherPromise = promise();
    this.stretcherPromise.then((node) => {
      this.node = node;
    });

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
  }

  connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    const action = (node: IStretcherNode) => {
      if (destination instanceof AudioParam) {
        node.connect(destination.param);
        return;
      }
      node.connect(destination.node);
    };

    if (!this.node) {
      this.stretcherPromise!.then((node) => {
        action(node);
      });
    } else {
      action(this.node);
    }

    return destination;
  }

  disconnect(destination?: AudioNode | AudioParam): void {
    const action = (node: IStretcherNode) => {
      if (destination === undefined) {
        node.disconnect();
        return;
      }

      if (destination instanceof AudioParam) {
        node.disconnect(destination.param);
        return;
      }
      node.disconnect(destination.node);
    };

    if (!this.node) {
      this.stretcherPromise!.then((node) => {
        action(node);
      });
    } else {
      action(this.node);
    }
  }

  start(when?: number, offset?: number, duration?: number): void {
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
    const startAt =
      !when || when < this.context.currentTime
        ? this.context.currentTime
        : when;

    const scheduleAction = (node: IStretcherNode) => {
      node.schedule({
        loopStart: this._loopStart,
        loopEnd: this._loopEnd,
      });
    };

    if (this.loop && this._loopStart !== -1 && this._loopEnd !== -1) {
      if (!this.node) {
        this.stretcherPromise!.then((node) => {
          scheduleAction(node);
        });
      } else {
        scheduleAction(this.node);
      }
    }

    const startAction = (node: IStretcherNode) => {
      node.start(
        startAt,
        offset,
        duration,
        this.playbackRate.value,
        Math.floor(clamp(this.detune.value / 100, -12, 12))
      );
    };
    if (!this.node) {
      this.stretcherPromise!.then((node) => {
        startAction(node);
      });
    } else {
      startAction(this.node);
    }
  }

  stop(when: number): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }
    const action = (node: IStretcherNode) => {
      node.stop(when);
    };
    if (!this.node) {
      this.stretcherPromise!.then((node) => {
        action(node);
      });
      return;
    }
    action(this.node);
  }

  setDetune(value: number, when?: number): void {
    if (!this.hasBeenStarted) {
      return;
    }
    const action = (node: IStretcherNode) => {
      node.schedule({
        semitones: Math.floor(clamp(value / 100, -12, 12)),
        output: when,
      });
    };

    if (!this.node) {
      this.stretcherPromise!.then((node) => {
        action(node);
      });
      return;
    }

    action(this.node);
  }

  setPlaybackRate(value: number, when?: number): void {
    if (!this.hasBeenStarted) {
      return;
    }
    const action = (node: IStretcherNode) => {
      node.schedule({
        rate: value,
        output: when,
      });
    };

    if (!this.node) {
      this.stretcherPromise!.then((node) => {
        action(node);
      });
      return;
    }

    action(this.node);
  }

  get buffer(): AudioBuffer | null {
    return this._buffer;
  }

  set buffer(buffer: AudioBuffer | null) {
    this._buffer = buffer;

    const action = (node: IStretcherNode) => {
      node.dropBuffers();

      if (!buffer) {
        return;
      }

      const channelArrays: Float32Array[] = [];

      for (let i = 0; i < buffer.numberOfChannels; i++) {
        channelArrays.push(buffer.getChannelData(i));
      }

      node.addBuffers(channelArrays);
    };

    if (!this.node) {
      this.stretcherPromise!.then((node) => {
        action(node);
      });
      return;
    }
    action(this.node);
  }

  get loop(): boolean {
    return this._loop;
  }

  set loop(value: boolean) {
    this._loop = value;
  }

  get loopStart(): number {
    return this._loopStart;
  }

  set loopStart(value: number) {
    this._loopStart = value;
  }

  get loopEnd(): number {
    return this._loopEnd;
  }

  set loopEnd(value: number) {
    this._loopEnd = value;
  }
}

class AudioBufferSourceNodeWeb implements IAudioAPIBufferSourceNodeWeb {
  private node: DefaultSource;
  private hasBeenStarted: boolean = false;
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  constructor(context: BaseAudioContext, options?: AudioBufferSourceOptions) {
    this.node = new globalThis.AudioBufferSourceNode(context.context, {
      ...options,
      ...(options?.buffer ? { buffer: options.buffer.buffer } : {}),
    });
    this.detune = new AudioParam(this.node.detune, context);
    this.playbackRate = new AudioParam(this.node.playbackRate, context);
  }

  start(when: number = 0, offset?: number, duration?: number): void {
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
    this.node.start(when, offset, duration);
  }

  stop(when: number = 0): void {
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
    this.node.stop(when);
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  setDetune(value: number, when?: number): void {
    console.warn('setDetune is not implemented for non-pitch-correction mode');
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  setPlaybackRate(value: number, when?: number): void {
    console.warn(
      'setPlaybackRate is not implemented for non-pitch-correction mode'
    );
  }

  get buffer(): AudioBuffer | null {
    const buffer = this.node.buffer;
    if (!buffer) {
      return null;
    }

    return new AudioBuffer(buffer);
  }

  set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      this.node.buffer = null;
      return;
    }

    this.node.buffer = buffer.buffer;
  }

  get loop(): boolean {
    return this.node.loop;
  }

  set loop(value: boolean) {
    this.node.loop = value;
  }

  get loopStart(): number {
    return this.node.loopStart;
  }

  set loopStart(value: number) {
    this.node.loopStart = value;
  }

  get loopEnd(): number {
    return this.node.loopEnd;
  }

  set loopEnd(value: number) {
    this.node.loopEnd = value;
  }

  connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    if (destination instanceof AudioParam) {
      this.node.connect(destination.param);
    } else {
      this.node.connect(destination.node);
    }
    return destination;
  }

  disconnect(destination?: AudioNode | AudioParam): void {
    if (destination === undefined) {
      this.node.disconnect();
      return;
    }

    if (destination instanceof AudioParam) {
      this.node.disconnect(destination.param);
      return;
    }
    this.node.disconnect(destination.node);
  }
}

export default class AudioBufferSourceNode
  implements IAudioAPIBufferSourceNodeWeb
{
  private node: AudioBufferSourceNodeStretcher | AudioBufferSourceNodeWeb;
  constructor(context: BaseAudioContext, options?: AudioBufferSourceOptions) {
    this.node = options?.pitchCorrection
      ? new AudioBufferSourceNodeStretcher(context)
      : new AudioBufferSourceNodeWeb(context, options);
  }

  connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    return this.asAudioBufferSourceNodeWeb().connect(destination);
  }

  disconnect(destination?: AudioNode | AudioParam): void {
    this.asAudioBufferSourceNodeWeb().disconnect(destination);
  }

  asAudioBufferSourceNodeWeb(): IAudioAPIBufferSourceNodeWeb {
    return this.node as unknown as IAudioAPIBufferSourceNodeWeb;
  }

  start(when: number = 0, offset?: number, duration?: number): void {
    this.asAudioBufferSourceNodeWeb().start(when, offset, duration);
  }

  stop(when: number = 0): void {
    this.asAudioBufferSourceNodeWeb().stop(when);
  }

  setDetune(value: number, when?: number): void {
    this.asAudioBufferSourceNodeWeb().setDetune(value, when);
  }

  setPlaybackRate(value: number, when?: number): void {
    this.asAudioBufferSourceNodeWeb().setPlaybackRate(value, when);
  }

  get buffer(): AudioBuffer | null {
    return this.asAudioBufferSourceNodeWeb().buffer;
  }

  set buffer(buffer: AudioBuffer | null) {
    this.asAudioBufferSourceNodeWeb().buffer = buffer;
  }

  get loop(): boolean {
    return this.asAudioBufferSourceNodeWeb().loop;
  }

  set loop(value: boolean) {
    this.asAudioBufferSourceNodeWeb().loop = value;
  }

  get loopStart(): number {
    return this.asAudioBufferSourceNodeWeb().loopStart;
  }

  set loopStart(value: number) {
    this.asAudioBufferSourceNodeWeb().loopStart = value;
  }

  get loopEnd(): number {
    return this.asAudioBufferSourceNodeWeb().loopEnd;
  }

  set loopEnd(value: number) {
    this.asAudioBufferSourceNodeWeb().loopEnd = value;
  }
}

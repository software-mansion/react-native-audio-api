import { InvalidStateError, RangeError } from '../../../errors';

import AudioParam from '../../AudioParam.web';
import AudioBuffer from '../../AudioBuffer.web';
import BaseAudioContext from '../../BaseAudioContext.web';
import AudioNode from '../../AudioNode.web';

import { clamp } from '../../../utils';
import { AudioBufferSourceOptions } from '../../../types';
import SignalsmithStretch from './signalsmithStretch/SignalsmithStretch.js';

import { AudioBufferSourceNodeBackend } from '../../types.web';
import { ScheduleOptions, WasmAudioBufferSourceStretcherNode } from './types';
import AudioStretcherParam, {
  StretcherAutomationEvent,
} from './AudioStretcherParam';

function buildScheduleOptions(
  options: Omit<ScheduleOptions, 'output'>,
  time?: number
): ScheduleOptions {
  if (time === undefined) {
    return options;
  }

  return { ...options, output: time };
}

function detuneCentsToSemitones(cents: number): number {
  return clamp(cents / 100, -12, 12);
}

type PendingParamEvent = {
  param: AudioStretcherParam;
  event: StretcherAutomationEvent;
  mapOptions: (value: number) => Omit<ScheduleOptions, 'output'>;
};

export default class AudioBufferSourceNodeStretcher implements AudioBufferSourceNodeBackend {
  private stretcherPromise: Promise<WasmAudioBufferSourceStretcherNode> | null =
    null;

  private node: WasmAudioBufferSourceStretcherNode | null = null;
  private hasBeenStarted: boolean = false;
  private context: BaseAudioContext;
  readonly detune: AudioStretcherParam;
  readonly playbackRate: AudioStretcherParam;

  private _loop: boolean = false;
  private _loopStart: number = -1;
  private _loopEnd: number = -1;
  private _loopSkip: boolean = false;
  private _onLoopEnded: ((event: object) => void) | undefined = undefined;

  private _buffer: AudioBuffer | null = null;
  private bufferHasBeenSet: boolean = false;
  private _operationChain: Promise<void> = Promise.resolve();
  private _pendingParamEvents: PendingParamEvent[] = [];

  constructor(context: BaseAudioContext, options: AudioBufferSourceOptions) {
    this.context = context;
    const stretcherPromise = SignalsmithStretch(context.context);
    this.stretcherPromise = stretcherPromise;
    stretcherPromise.then((node) => {
      this.node = node;
    });

    this.detune = new AudioStretcherParam(
      context,
      options.detune ?? 0,
      0,
      -1200,
      1200,
      (event) => {
        this.handleParamEvent(this.detune, event, (value) => ({
          semitones: detuneCentsToSemitones(value),
        }));
      },
      (cancelTime) => {
        this.cancelStretcherAutomation(cancelTime);
      }
    );
    this.playbackRate = new AudioStretcherParam(
      context,
      options.playbackRate ?? 1,
      1,
      0,
      Infinity,
      (event) => {
        this.handleParamEvent(this.playbackRate, event, (value) => ({
          rate: value,
        }));
      },
      (cancelTime) => {
        this.cancelStretcherAutomation(cancelTime);
      }
    );
    this.buffer = (options.buffer as AudioBuffer) ?? null;
  }

  private runOnStretcher(
    action: (node: WasmAudioBufferSourceStretcherNode) => void
  ): Promise<void> {
    this._operationChain = this._operationChain.then(() =>
      this.stretcherPromise!.then(action)
    );
    return this._operationChain;
  }

  private scheduleStretcher(
    options: Omit<ScheduleOptions, 'output'>,
    time?: number
  ): void {
    const scheduleOptions = buildScheduleOptions(options, time);
    this.runOnStretcher((node) => {
      node.schedule(scheduleOptions);
    });
  }

  private cancelStretcherAutomation(cancelTime: number): void {
    this._pendingParamEvents = this._pendingParamEvents.filter(
      (pending) => pending.event.time < cancelTime
    );
    this.runOnStretcher((node) => {
      node.cancelScheduledValues(cancelTime);
    });
  }

  private handleParamEvent(
    param: AudioStretcherParam,
    event: StretcherAutomationEvent,
    mapOptions: (value: number) => Omit<ScheduleOptions, 'output'>
  ): void {
    if (!this.hasBeenStarted) {
      this._pendingParamEvents.push({
        param,
        event,
        mapOptions,
      });
      return;
    }

    this.applyParamEvent(param, event, mapOptions);
  }

  private applyParamEvent(
    param: AudioStretcherParam,
    event: StretcherAutomationEvent,
    mapOptions: (value: number) => Omit<ScheduleOptions, 'output'>
  ): void {
    const samples = param.sampleAutomation(event, (value) => value);
    for (const sample of samples) {
      this.scheduleStretcher(mapOptions(sample.value), sample.time);
    }
  }

  private flushPendingParamEvents(): void {
    const pending = this._pendingParamEvents;
    this._pendingParamEvents = [];
    for (const { param, event, mapOptions } of pending) {
      this.applyParamEvent(param, event, mapOptions);
    }
  }

  connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    const action = (node: WasmAudioBufferSourceStretcherNode) => {
      if (destination instanceof AudioParam) {
        node.connect(destination.param);
        return;
      }
      node.connect(destination.node);
    };

    this.runOnStretcher(action);
    return destination;
  }

  disconnect(destination?: AudioNode | AudioParam): void {
    const action = (node: WasmAudioBufferSourceStretcherNode) => {
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

    this.runOnStretcher(action);
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

    this.runOnStretcher((node) => {
      if (this.loop && this._loopStart !== -1 && this._loopEnd !== -1) {
        node.schedule({
          loopStart: this._loopStart,
          loopEnd: this._loopEnd,
        });
      }

      const playbackRate = this.playbackRate.getValueAtTime(startAt);
      const detune = this.detune.getValueAtTime(startAt);
      node.start(
        startAt,
        offset,
        duration,
        playbackRate,
        detuneCentsToSemitones(detune)
      );
      this.flushPendingParamEvents();
    });
  }

  stop(when: number): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }
    this.runOnStretcher((node) => {
      node.stop(when);
    });
  }

  get buffer(): AudioBuffer | null {
    return this._buffer;
  }

  set buffer(buffer: AudioBuffer | null) {
    if (buffer !== null && this.bufferHasBeenSet) {
      throw new InvalidStateError(
        'The buffer can only be set once and cannot be changed afterwards.'
      );
    }

    this._buffer = buffer;
    if (buffer !== null) {
      this.bufferHasBeenSet = true;
    }

    this.runOnStretcher((node) => {
      node.dropBuffers();

      if (!buffer) {
        return;
      }

      const channelArrays: Float32Array[] = [];

      for (let i = 0; i < buffer.numberOfChannels; i++) {
        channelArrays.push(buffer.getChannelData(i));
      }

      node.addBuffers(channelArrays);
    });
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

  get loopSkip(): boolean {
    return this._loopSkip;
  }

  set loopSkip(value: boolean) {
    this._loopSkip = value;
  }

  get onLoopEnded(): ((event: object) => void) | undefined {
    return this._onLoopEnded;
  }

  // The WASM stretcher has no per-loop event; callback is stored but never fired.
  set onLoopEnded(callback: ((event: object) => void) | null) {
    this._onLoopEnded = callback ?? undefined;
  }
}

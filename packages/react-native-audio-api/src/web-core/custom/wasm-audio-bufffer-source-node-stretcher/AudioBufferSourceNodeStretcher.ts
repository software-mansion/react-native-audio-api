import { InvalidStateError, RangeError } from '../../../errors';

import AudioParam from '../../AudioParam.web';
import AudioBuffer from '../../AudioBuffer.web';
import BaseAudioContext from '../../BaseAudioContext.web';
import AudioNode from '../../AudioNode.web';

import { clamp } from '../../../utils';
import { AudioBufferSourceOptions } from '../../../types';
import LoadCustomWasm, { globalWasmPromise, globalTag } from './LoadCustomWasm';

import { AudioBufferSourceNodeBackend } from '../../types.web';
import {
  ScheduleOptions,
  WasmAudioBufferSourceStretcherNode,
  WasmAudioBufferSourceStretcherNodeFactory,
} from './types';
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

  private _onEnded: ((event: Event) => void) | null = null;

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
    // The worklet is loaded as a static asset (served next to the app) rather
    // than bundled, so it is never transformed by the consumer's bundler. The
    // consumer preloads it via LoadCustomWasm(prefix); this call is idempotent.
    const stretcherPromise = (async () => {
      await LoadCustomWasm('/react-native-audio-api');
      await globalWasmPromise;
      const factory = (
        window as unknown as Record<
          string,
          WasmAudioBufferSourceStretcherNodeFactory
        >
      )[globalTag];
      return factory(context.context);
    })();
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
      const playbackRate = this.playbackRate.getValueAtTime(startAt);
      const detune = this.detune.getValueAtTime(startAt);
      node.start(
        startAt,
        offset,
        duration,
        playbackRate,
        detuneCentsToSemitones(detune)
      );

      // Pin the loop to the same segment start() just scheduled (output=startAt).
      // A separate schedule() with no output would land at the worklet's own
      // (later) currentTime as an inactive trailing segment and silence playback.
      if (this.loop && this._loopStart !== -1 && this._loopEnd !== -1) {
        node.schedule({
          output: startAt,
          loopStart: this._loopStart,
          loopEnd: this._loopEnd,
        });
      }

      this.flushPendingParamEvents();
    });
  }

  stop(when?: number): void {
    if (when !== undefined && when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }
    // Passing `undefined` lets the worklet stop at its own fresh currentTime.
    // Passing 0 would schedule deactivation before the start segment (a no-op).
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

  // Unlike the browser AudioBufferSourceNode, the WASM worklet only picks up
  // loop bounds when they are scheduled. start() schedules the initial loop;
  // these setters must re-schedule so live changes take effect while playing.
  private applyLoopToStretcher(): void {
    if (!this.hasBeenStarted) {
      return;
    }
    this.runOnStretcher((node) => {
      if (this._loop && this._loopStart !== -1 && this._loopEnd !== -1) {
        node.schedule({
          loopStart: this._loopStart,
          loopEnd: this._loopEnd,
        });
      } else {
        // Zero-length loop disables looping in the worklet going forward.
        node.schedule({ loopStart: 0, loopEnd: 0 });
      }
    });
  }

  get loop(): boolean {
    return this._loop;
  }

  set loop(value: boolean) {
    this._loop = value;
    this.applyLoopToStretcher();
  }

  get loopStart(): number {
    return this._loopStart;
  }

  set loopStart(value: number) {
    this._loopStart = value;
    this.applyLoopToStretcher();
  }

  get loopEnd(): number {
    return this._loopEnd;
  }

  set loopEnd(value: number) {
    this._loopEnd = value;
    this.applyLoopToStretcher();
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

  get onEnded(): ((event: Event) => void) | null {
    return this._onEnded;
  }

  set onEnded(callback: ((event: Event) => void) | null) {
    this._onEnded = callback;
    this.runOnStretcher((node) => {
      node.onEnded = callback;
    });
  }
}

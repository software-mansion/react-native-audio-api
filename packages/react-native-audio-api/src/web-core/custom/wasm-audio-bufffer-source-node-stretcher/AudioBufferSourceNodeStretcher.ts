import { InvalidStateError, RangeError } from '../../../errors';

import AudioParam from '../../AudioParam.web';
import AudioBuffer from '../../AudioBuffer.web';
import BaseAudioContext from '../../BaseAudioContext.web';
import AudioNode from '../../AudioNode.web';

import { clamp } from '../../../utils';
import { AudioBufferSourceOptions } from '../../../types';
import SignalsmithStretch from './signalsmithStretch/SignalsmithStretch.js';

import { AudioBufferSourceNodeBackend } from '../../types.web';
import { WasmAudioBufferSourceStretcherNode } from './types';
import AudioStretcherParam from './AudioStretcherParam';

export default class AudioBufferSourceNodeStretcher implements AudioBufferSourceNodeBackend {
  private stretcherPromise: Promise<WasmAudioBufferSourceStretcherNode> | null =
    null;

  private node: WasmAudioBufferSourceStretcherNode | null = null;
  private hasBeenStarted: boolean = false;
  private context: BaseAudioContext;
  readonly detune: AudioParam;
  readonly playbackRate: AudioParam;

  private _loop: boolean = false;
  private _loopStart: number = -1;
  private _loopEnd: number = -1;
  private _loopSkip: boolean = false;
  private _onLoopEnded: ((event: object) => void) | undefined = undefined;

  private _buffer: AudioBuffer | null = null;
  private bufferHasBeenSet: boolean = false;

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
      (value, time) => {
        if (!this.hasBeenStarted) return;
        const action = (node: WasmAudioBufferSourceStretcherNode) => {
          node.schedule({
            semitones: Math.floor(clamp(value / 100, -12, 12)),
            output: time,
          });
        };
        if (!this.node) {
          this.stretcherPromise!.then(action);
        } else {
          action(this.node);
        }
      }
    );
    this.playbackRate = new AudioStretcherParam(
      context,
      options.playbackRate ?? 1,
      1,
      0,
      Infinity,
      (value, time) => {
        if (!this.hasBeenStarted) return;
        const action = (node: WasmAudioBufferSourceStretcherNode) => {
          node.schedule({ rate: value, output: time });
        };
        if (!this.node) {
          this.stretcherPromise!.then(action);
        } else {
          action(this.node);
        }
      }
    );
    this.buffer = (options.buffer as AudioBuffer) ?? null;
  }

  connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    const action = (node: WasmAudioBufferSourceStretcherNode) => {
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

    const scheduleAction = (node: WasmAudioBufferSourceStretcherNode) => {
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

    const startAction = (node: WasmAudioBufferSourceStretcherNode) => {
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
    const action = (node: WasmAudioBufferSourceStretcherNode) => {
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

    const action = (node: WasmAudioBufferSourceStretcherNode) => {
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

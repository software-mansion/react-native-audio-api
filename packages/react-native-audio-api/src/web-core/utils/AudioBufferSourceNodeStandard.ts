import { InvalidStateError } from '../../errors';
import AudioBuffer from '../AudioBuffer.web';
import AudioNode from '../AudioNode.web';
import AudioParam from '../AudioParam.web';
import BaseAudioContext from '../BaseAudioContext.web';
import { AudioBufferSourceNodeBackend } from '../types.web';

export default class AudioBufferSourceNodeStandard implements AudioBufferSourceNodeBackend {
  private node: globalThis.AudioBufferSourceNode;
  private hasBeenStarted: boolean = false;
  private _loopSkip: boolean = false;
  private _onLoopEnded: ((event: object) => void) | undefined = undefined;
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  public constructor(
    context: BaseAudioContext,
    options?: AudioBufferSourceOptions
  ) {
    const { buffer, ...rest } = options ?? {};
    this.node = new globalThis.AudioBufferSourceNode(context.context, {
      ...rest,
      ...(buffer ? { buffer: (buffer as AudioBuffer).buffer } : {}),
    });
    this.detune = new AudioParam(this.node.detune, context);
    this.playbackRate = new AudioParam(this.node.playbackRate, context);
  }

  public start(when: number = 0, offset?: number, duration?: number): void {
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
    this.node.stop(when);
  }

  public connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    if (destination instanceof AudioParam) {
      this.node.connect(destination.param);
    } else {
      this.node.connect(destination.node);
    }
    return destination;
  }

  public disconnect(destination?: AudioNode | AudioParam): void {
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

  public get buffer(): AudioBuffer | null {
    const buffer = this.node.buffer;
    if (!buffer) {
      return null;
    }

    return new AudioBuffer(buffer);
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      this.node.buffer = null;
      return;
    }

    this.node.buffer = buffer.buffer;
  }

  public get loop(): boolean {
    return this.node.loop;
  }

  public set loop(value: boolean) {
    this.node.loop = value;
  }

  public get loopStart(): number {
    return this.node.loopStart;
  }

  public set loopStart(value: number) {
    this.node.loopStart = value;
  }

  public get loopEnd(): number {
    return this.node.loopEnd;
  }

  public set loopEnd(value: number) {
    this.node.loopEnd = value;
  }

  public get loopSkip(): boolean {
    return this._loopSkip;
  }

  public set loopSkip(value: boolean) {
    this._loopSkip = value;
  }

  public get onLoopEnded(): ((event: object) => void) | undefined {
    return this._onLoopEnded;
  }

  // The browser Web Audio API has no per-loop event; callback is stored but never fired.
  public set onLoopEnded(callback: ((event: object) => void) | null) {
    this._onLoopEnded = callback ?? undefined;
  }
}

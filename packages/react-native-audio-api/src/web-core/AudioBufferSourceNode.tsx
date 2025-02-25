import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import AudioParam from './AudioParam';
import AudioBuffer from './AudioBuffer';
import { InvalidStateError, RangeError } from '../errors';
import BaseAudioContext from './BaseAudioContext';
import { TimeStretchType } from '../types';

export default class AudioBufferSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  constructor(
    context: BaseAudioContext,
    node: globalThis.AudioBufferSourceNode
  ) {
    super(context, node);

    this.detune = new AudioParam(node.detune);
    this.playbackRate = new AudioParam(node.playbackRate);
  }

  public get buffer(): AudioBuffer | null {
    const buffer = (this.node as globalThis.AudioBufferSourceNode).buffer;

    if (!buffer) {
      return null;
    }

    return new AudioBuffer(buffer);
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      (this.node as globalThis.AudioBufferSourceNode | null) = null;
      return;
    }

    (this.node as globalThis.AudioBufferSourceNode).buffer = buffer.buffer;
  }

  public get loop(): boolean {
    return (this.node as globalThis.AudioBufferSourceNode).loop;
  }

  public set loop(value: boolean) {
    (this.node as globalThis.AudioBufferSourceNode).loop = value;
  }

  public get loopStart(): number {
    return (this.node as globalThis.AudioBufferSourceNode).loopStart;
  }

  public set loopStart(value: number) {
    (this.node as globalThis.AudioBufferSourceNode).loopStart = value;
  }

  public get loopEnd(): number {
    return (this.node as globalThis.AudioBufferSourceNode).loopEnd;
  }

  public set loopEnd(value: number) {
    (this.node as globalThis.AudioBufferSourceNode).loopEnd = value;
  }

  public get timeStretch(): TimeStretchType {
    return 'linear';
  }

  public set timeStretch(value: TimeStretchType) {
    console.log(
      'React Native Audio API: setting timeStretch is not supported on web'
    );
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
    (this.node as globalThis.AudioBufferSourceNode).start(
      when,
      offset,
      duration
    );
  }
}

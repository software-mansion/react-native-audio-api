import { IAudioBufferSourceNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';
import AudioBuffer from './AudioBuffer';
import AudioParam from './AudioParam';
import { InvalidStateError, RangeError } from '../errors';

export default class AudioBufferSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  constructor(context: BaseAudioContext, node: IAudioBufferSourceNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune);
    this.playbackRate = new AudioParam(node.playbackRate);
  }

  public get buffer(): AudioBuffer | null {
    const buffer = (this.node as IAudioBufferSourceNode).buffer;
    if (!buffer) {
      return null;
    }
    return new AudioBuffer(buffer);
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      (this.node as IAudioBufferSourceNode).buffer = null;
      return;
    }

    (this.node as IAudioBufferSourceNode).buffer = buffer.buffer;
  }

  public get loop(): boolean {
    return (this.node as IAudioBufferSourceNode).loop;
  }

  public set loop(value: boolean) {
    (this.node as IAudioBufferSourceNode).loop = value;
  }

  public get loopStart(): number {
    return (this.node as IAudioBufferSourceNode).loopStart;
  }

  public set loopStart(value: number) {
    (this.node as IAudioBufferSourceNode).loopStart = value;
  }

  public get loopEnd(): number {
    return (this.node as IAudioBufferSourceNode).loopEnd;
  }

  public set loopEnd(value: number) {
    (this.node as IAudioBufferSourceNode).loopEnd = value;
  }

  public start(when: number = 0, offset: number = 0, duration?: number): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (offset < 0) {
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
    (this.node as IAudioBufferSourceNode).start(when, offset, duration);
  }
}

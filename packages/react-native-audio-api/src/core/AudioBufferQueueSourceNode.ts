import { IAudioBufferQueueSourceNode } from '../interfaces';
import AudioBufferBaseSourceNode from './AudioBufferBaseSourceNode';
import AudioBuffer from './AudioBuffer';
import { RangeError } from '../errors';

export default class AudioBufferQueueSourceNode extends AudioBufferBaseSourceNode {
  public enqueueBuffer(buffer: AudioBuffer): string {
    return (this.node as IAudioBufferQueueSourceNode).enqueueBuffer(
      buffer.buffer
    );
  }

  public dequeueBuffer(bufferId: string): void {
    const id = parseInt(bufferId, 10);
    if (isNaN(id) || id < 0) {
      throw new RangeError(
        `bufferId must be a non-negative integer: ${bufferId}`
      );
    }
    (this.node as IAudioBufferQueueSourceNode).dequeueBuffer(id);
  }

  public clearBuffers(): void {
    (this.node as IAudioBufferQueueSourceNode).clearBuffers();
  }

  public override start(when: number = 0, offset?: number): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (offset && offset < 0) {
      throw new RangeError(
        `offset must be a finite non-negative number: ${offset}`
      );
    }

    (this.node as IAudioBufferQueueSourceNode).start(when, offset);
  }

  public override stop(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    (this.node as IAudioBufferQueueSourceNode).stop(when);
  }

  public pause(): void {
    (this.node as IAudioBufferQueueSourceNode).pause();
  }
}

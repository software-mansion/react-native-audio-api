import { IAudioBufferQueueSourceNode } from '../interfaces';
import AudioBufferBaseSourceNode from './AudioBufferBaseSourceNode';
import AudioBuffer from './AudioBuffer';
import { RangeError } from '../errors';

export default class AudioBufferQueueSourceNode extends AudioBufferBaseSourceNode {
  public enqueueBuffer(
    buffer: AudioBuffer,
    isLastBuffer: boolean = false
  ): void {
    (this.node as IAudioBufferQueueSourceNode).enqueueBuffer(
      buffer.buffer,
      isLastBuffer
    );
  }

  public override start(when: number = 0, offset?: number): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (offset) {
      if (offset < 0) {
        throw new RangeError(
          `offset must be a finite non-negative number: ${offset}`
        );
      }
    }

    (this.node as IAudioBufferQueueSourceNode).start(when);
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

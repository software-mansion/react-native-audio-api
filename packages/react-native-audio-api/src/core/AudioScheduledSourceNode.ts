import { IAudioScheduledSourceNode } from '../interfaces';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';
import { InvalidStateError, RangeError } from '../errors';

export default class AudioScheduledSourceNode extends AudioNode {
  private hasBeenStarted: boolean = false;

  constructor(context: BaseAudioContext, node: IAudioScheduledSourceNode) {
    super(context, node);
  }

  public start(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${when}`
      );
    }

    if (this.hasBeenStarted) {
      throw new InvalidStateError('Cannot call start more than once');
    }

    this.hasBeenStarted = true;
    (this.node as IAudioScheduledSourceNode).start(when);
  }

  public stop(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${when}`
      );
    }

    if (!this.hasBeenStarted) {
      throw new InvalidStateError(
        'Cannot call stop without calling start first'
      );
    }

    (this.node as IAudioScheduledSourceNode).stop(when);
  }
}

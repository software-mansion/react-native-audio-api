import AudioNode from './AudioNode';
import { RangeError, InvalidStateError } from '../errors';

export default class AudioScheduledSourceNode extends AudioNode {
  protected hasBeenStarted: boolean = false;

  public start(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (this.hasBeenStarted) {
      throw new InvalidStateError('Cannot call start more than once');
    }

    this.hasBeenStarted = true;
    (this.node as globalThis.AudioScheduledSourceNode).start(when);
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

    (this.node as globalThis.AudioScheduledSourceNode).stop(when);
  }

  // eslint-disable-next-line accessor-pairs
  public set onended(callback: (arg?: number) => void) {
    const eventCallback = (_event: Event) => {
      callback();
    };

    (this.node as globalThis.AudioScheduledSourceNode).onended = eventCallback;
  }
}

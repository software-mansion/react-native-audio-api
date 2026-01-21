import AudioNode from './AudioNode';
import { EventEmptyType } from '../events/types';
import { RangeError, InvalidStateError } from '../errors';

export default class AudioScheduledSourceNode extends AudioNode {
  protected hasBeenStarted: boolean = false;
  private onEndedCallback?: (event: EventEmptyType) => void;

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

  public get onEnded(): ((event: EventEmptyType) => void) | undefined {
    return this.onEndedCallback;
  }

  public set onEnded(callback: (event: EventEmptyType) => void | null) {
    (this.node as globalThis.AudioScheduledSourceNode).onended = callback;
    this.onEndedCallback = callback;
  }
}

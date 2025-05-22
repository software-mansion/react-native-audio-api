import { IAudioScheduledSourceNode } from '../interfaces';
import AudioNode from './AudioNode';
import { InvalidStateError, RangeError } from '../errors';
import { OnEndedEventType } from '../events/types';
import { AudioEventEmitter } from '../events';

export default class AudioScheduledSourceNode extends AudioNode {
  protected hasBeenStarted: boolean = false;
  private readonly audioEventEmitter = new AudioEventEmitter(
    global.AudioEventEmitter
  );

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
    (this.node as IAudioScheduledSourceNode).start(when);
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

    (this.node as IAudioScheduledSourceNode).stop(when);
  }

  // eslint-disable-next-line accessor-pairs
  public set onended(callback: (event: OnEndedEventType) => void) {
    const subscription = this.audioEventEmitter.addAudioEventListener(
      'ended',
      callback
    );

    (this.node as IAudioScheduledSourceNode).onended =
      subscription.subscriptionId;
  }
}

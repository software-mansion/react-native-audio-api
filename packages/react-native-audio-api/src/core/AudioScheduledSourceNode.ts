import { IAudioScheduledSourceNode } from '../interfaces';
import AudioNode from './AudioNode';
import { InvalidStateError, RangeError } from '../errors';
import { OnEndedEventType } from '../events/types';
import { AudioEventEmitter, AudioEventSubscription } from '../events';

export default class AudioScheduledSourceNode extends AudioNode {
  protected hasBeenStarted: boolean = false;
  protected readonly audioEventEmitter = new AudioEventEmitter(
    global.AudioEventEmitter
  );

  private onEndedSubscription?: AudioEventSubscription;
  private onEndedCallback?: (event: OnEndedEventType) => void;

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

  public get onEnded(): ((event: OnEndedEventType) => void) | undefined {
    return this.onEndedCallback;
  }

  public set onEnded(callback: ((event: OnEndedEventType) => void) | null) {
    if (!callback) {
      (this.node as IAudioScheduledSourceNode).onEnded = '0';
      this.onEndedSubscription?.remove();
      this.onEndedSubscription = undefined;
      this.onEndedCallback = undefined;
      return;
    }

    this.onEndedCallback = callback;
    this.onEndedSubscription = this.audioEventEmitter.addAudioEventListener(
      'ended',
      callback
    );

    (this.node as IAudioScheduledSourceNode).onEnded =
      this.onEndedSubscription.subscriptionId;
  }
}

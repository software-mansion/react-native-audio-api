import { IAudioScheduledSourceNode } from '../interfaces';
import AudioNode from './AudioNode';
import { InvalidStateError, RangeError } from '../errors';
import { EventEmptyType } from '../events/types';
import { AudioEventEmitter, AudioEventSubscription } from '../events';

export default class AudioScheduledSourceNode extends AudioNode {
  protected hasBeenStarted: boolean = false;
  protected readonly audioEventEmitter = new AudioEventEmitter(
    global.AudioEventEmitter
  );

  private onendedSubscription?: AudioEventSubscription;
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

  public get onended(): ((event: EventEmptyType) => void) | undefined {
    return this.onEndedCallback;
  }

  public set onended(callback: ((event: EventEmptyType) => void) | null) {
    if (!callback) {
      (this.node as IAudioScheduledSourceNode).onended = '0';
      this.onendedSubscription?.remove();
      this.onendedSubscription = undefined;
      this.onEndedCallback = undefined;
      return;
    }

    this.onEndedCallback = callback;
    this.onendedSubscription = this.audioEventEmitter.addAudioEventListener(
      'ended',
      callback
    );

    (this.node as IAudioScheduledSourceNode).onended =
      this.onendedSubscription.subscriptionId;
  }
}

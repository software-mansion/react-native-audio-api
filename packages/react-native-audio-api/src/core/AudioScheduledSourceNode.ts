import { IAudioScheduledSourceNode } from '../jsi-interfaces';
import AudioNode from './AudioNode';
import { InvalidStateError, RangeError } from '../errors';
import { EventEmptyType } from '../events/types';
import { AudioEventEmitter, AudioEventSubscription } from '../events';

export default class AudioScheduledSourceNode extends AudioNode {
  protected hasBeenStarted: boolean = false;
  protected readonly audioEventEmitter = new AudioEventEmitter(
    globalThis.AudioEventEmitter
  );

  private onEndedCallback?: (event: EventEmptyType) => void;
  private endedListeners = new Set<(event: EventEmptyType) => void>();
  private endedSubscription: AudioEventSubscription | null = null;

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
    this.context.markRunningOnSourceStart();
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

  /**
   * Web Audio API spec spelling of the ended-event handler. Delegates to
   * `onEnded` so both spellings drive the same native subscription.
   */
  public get onended(): ((event: EventEmptyType) => void) | undefined {
    return this.onEnded;
  }

  public set onended(callback: ((event: EventEmptyType) => void) | null) {
    this.onEnded = callback;
  }

  public get onEnded(): ((event: EventEmptyType) => void) | undefined {
    return this.onEndedCallback;
  }

  public set onEnded(callback: ((event: EventEmptyType) => void) | null) {
    this.onEndedCallback = callback ?? undefined;
    this.syncEndedSubscription();
  }

  /**
   * EventTarget-style registration for the `ended` event, sharing one native
   * subscription with the `onEnded`/`onended` handler. Other event types are
   * ignored: the node dispatches nothing else.
   */
  public addEventListener(
    type: string,
    listener: (event: EventEmptyType) => void
  ): void {
    if (type !== 'ended') {
      return;
    }

    this.endedListeners.add(listener);
    this.syncEndedSubscription();
  }

  public removeEventListener(
    type: string,
    listener: (event: EventEmptyType) => void
  ): void {
    if (type !== 'ended') {
      return;
    }

    this.endedListeners.delete(listener);
    this.syncEndedSubscription();
  }

  /**
   * Keep exactly one native `ended` subscription alive while any consumer
   * (handler or listener) exists, and none otherwise — an orphaned subscription
   * would retain this node in the native handler registry.
   */
  private syncEndedSubscription(): void {
    this.endedSubscription?.remove();
    this.endedSubscription = null;

    if (!this.onEndedCallback && this.endedListeners.size === 0) {
      (this.node as IAudioScheduledSourceNode).onEnded = '0';
      return;
    }

    this.endedSubscription = this.audioEventEmitter.addAudioEventListener(
      'ended',
      (event: EventEmptyType) => this.dispatchEnded(event)
    );
    (this.node as IAudioScheduledSourceNode).onEnded =
      this.endedSubscription.subscriptionId;
  }

  private dispatchEnded(event: EventEmptyType): void {
    const endedEvent = { ...event, type: 'ended', target: this };
    this.onEndedCallback?.(endedEvent);
    this.endedListeners.forEach((listener) => listener(endedEvent));
  }
}

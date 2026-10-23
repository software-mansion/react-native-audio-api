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

  private onendedCallback?: (event: EventEmptyType) => void;
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

  public get onended(): ((event: EventEmptyType) => void) | undefined {
    return this.onendedCallback;
  }

  public set onended(callback: ((event: EventEmptyType) => void) | null) {
    this.assertNotDisposed();
    this.onendedCallback = callback ?? undefined;
    this.syncEndedSubscription();
  }

  /**
   * EventTarget-style registration for the `ended` event, sharing one native
   * subscription with the `onended`/`onended` handler. Other event types are
   * ignored: the node dispatches nothing else.
   */
  public addEventListener(
    type: string,
    listener: (event: EventEmptyType) => void
  ): void {
    if (type !== 'ended') {
      return;
    }

    this.assertNotDisposed();
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

  protected clearEndedListeners(): void {
    this.onendedCallback = undefined;
    this.endedListeners.clear();
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

    if (!this.onendedCallback && this.endedListeners.size === 0) {
      (this.node as IAudioScheduledSourceNode).onended = '0';
      return;
    }

    this.endedSubscription = this.audioEventEmitter.addAudioEventListener(
      'ended',
      (event: EventEmptyType) => this.dispatchEnded(event)
    );
    (this.node as IAudioScheduledSourceNode).onended =
      this.endedSubscription.subscriptionId;
  }

  private dispatchEnded(event: EventEmptyType): void {
    const endedEvent = { ...event, type: 'ended', target: this };
    this.onendedCallback?.(endedEvent);
    this.endedListeners.forEach((listener) => listener(endedEvent));
  }
}

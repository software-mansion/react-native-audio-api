import { AudioEventName } from './types';
import { AudioEventEmitter } from './';

export default class AudioEventSubscription {
  private readonly audioEventEmitter: AudioEventEmitter;
  private readonly eventName: AudioEventName;
  /** @internal */
  public readonly subscriptionId: string;

  constructor(
    subscriptionId: string,
    eventName: AudioEventName,
    audioEventEmitter: AudioEventEmitter
  ) {
    this.subscriptionId = subscriptionId;
    this.eventName = eventName;
    this.audioEventEmitter = audioEventEmitter;
  }

  public remove(): void {
    this.audioEventEmitter.removeAudioEventListener(
      this.eventName,
      this.subscriptionId
    );
  }
}

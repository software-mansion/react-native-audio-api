import { AudioEventName } from './types';
import { AudioEventEmitter } from './';
import { NativeAudioAPIModule } from '../specs';

if (global.AudioEventEmitter == null) {
  if (!NativeAudioAPIModule) {
    throw new Error(
      `Failed to install react-native-audio-api: The native module could not be found.`
    );
  }

  NativeAudioAPIModule.install();
}

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

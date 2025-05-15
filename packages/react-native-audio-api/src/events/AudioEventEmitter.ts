import { AudioEventName, AudioEventCallback } from './types';
import AudioEventSubscription from './AudioEventSubscription';
import { IAudioEventEmitter } from '../interfaces';

export default class AudioEventEmitter {
  private readonly audioEventEmitter: IAudioEventEmitter;

  constructor(audioEventEmitter: IAudioEventEmitter) {
    this.audioEventEmitter = audioEventEmitter;
  }

  addAudioEventListener<Name extends AudioEventName>(
    name: Name,
    callback: AudioEventCallback<Name>
  ): AudioEventSubscription {
    const subscriptionId = this.audioEventEmitter.addAudioEventListener(
      name,
      callback
    );
    return new AudioEventSubscription(subscriptionId, name, this);
  }

  removeAudioEventListener<Name extends AudioEventName>(
    name: Name,
    subscriptionId: string
  ): void {
    this.audioEventEmitter.removeAudioEventListener(name, subscriptionId);
  }
}

import { AudioEventEmitter, AudioEventSubscription } from '../../events';
import { NativeAudioAPIModule } from '../../specs';
import type {
  RecordingNotificationEvent,
  NotificationManager,
  RecordingNotificationEventName,
  RecordingNotificationInfo,
} from './types';

import { AudioApiError } from '../../errors';

class RecordingNotificationManager
  implements
    NotificationManager<
      RecordingNotificationInfo,
      RecordingNotificationEventName
    >
{
  private notificationKey = 'react-native-audio-api-recording';
  private audioEventEmitter: AudioEventEmitter;

  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  /**
   * Show the notification with metadata or update if already visible.
   *
   * @param info - The info to be displayed.
   * @returns Promise that resolves after creating notification.
   */
  async show(info: RecordingNotificationInfo): Promise<void> {
    if (!NativeAudioAPIModule) {
      throw new AudioApiError('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.showNotification(
      'recording',
      this.notificationKey,
      info as Record<string, string | number | boolean | undefined>
    );

    if (result.error) {
      throw new AudioApiError(result.error);
    }
  }

  /**
   * Hide the notification.
   *
   * @returns Promise that resolves after hiding notification.
   */
  async hide(): Promise<void> {
    if (!NativeAudioAPIModule) {
      throw new AudioApiError('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.hideNotification(
      this.notificationKey
    );

    if (result.error) {
      throw new AudioApiError(result.error);
    }
  }

  /**
   * Check if the notification is currently active.
   *
   * @returns Promise that resolves to whether notification is active.
   */
  async isActive(): Promise<boolean> {
    if (!NativeAudioAPIModule) {
      return false;
    }

    return await NativeAudioAPIModule.isNotificationActive(
      this.notificationKey
    );
  }

  /**
   * Add an event listener for notification actions.
   *
   * @param eventName - The event name to listen for.
   * @param callback - The callback to invoke on event.
   * @returns Class that represents the subscription.
   */
  addEventListener<T extends RecordingNotificationEventName>(
    eventName: T,
    callback: (event: RecordingNotificationEvent[T]) => void
  ): AudioEventSubscription {
    return this.audioEventEmitter.addAudioEventListener(eventName, callback);
  }
}

export default new RecordingNotificationManager();

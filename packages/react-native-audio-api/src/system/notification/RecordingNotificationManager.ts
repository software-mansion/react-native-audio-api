import { AudioEventEmitter, AudioEventSubscription } from '../../events';
import { NativeAudioAPIModule } from '../../specs';
import type {
  RecordingNotificationEvent,
  NotificationManager,
  RecordingNotificationEventName,
  RecordingNotificationInfo,
} from './types';

import { NotSupportedError } from '../../errors';

/// Manager for media playback notifications with controls and MediaSession integration.
class RecordingNotificationManager
  implements
    NotificationManager<
      RecordingNotificationInfo,
      RecordingNotificationInfo,
      RecordingNotificationEventName
    >
{
  private notificationKey = 'react-native audio-api-recording';
  private audioEventEmitter: AudioEventEmitter;

  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  /// Show the notification with metadata or update if already visible.
  /// Automatically creates the notification on first call.
  async show(info: RecordingNotificationInfo): Promise<void> {
    if (!NativeAudioAPIModule) {
      throw new NotSupportedError('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.showNotification(
      'recording',
      this.notificationKey,
      info as Record<string, string | number | boolean | undefined>
    );

    if (result.error) {
      throw new Error(result.error);
    }
  }

  /// Update the notification with new metadata or state.
  /// This is an alias for show() since show handles both initial display and updates.
  async update(info: RecordingNotificationInfo): Promise<void> {
    return this.show(info);
  }

  /// Hide the notification.
  async hide(): Promise<void> {
    if (!NativeAudioAPIModule) {
      throw new NotSupportedError('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.hideNotification(
      this.notificationKey
    );

    if (result.error) {
      throw new Error(result.error);
    }
  }

  /// Check if the notification is currently active.
  async isActive(): Promise<boolean> {
    if (!NativeAudioAPIModule) {
      return false;
    }

    return await NativeAudioAPIModule.isNotificationActive(
      this.notificationKey
    );
  }

  /// Add an event listener for notification actions.
  addEventListener<T extends RecordingNotificationEventName>(
    eventName: T,
    callback: (event: RecordingNotificationEvent[T]) => void
  ): AudioEventSubscription {
    return this.audioEventEmitter.addAudioEventListener(eventName, callback);
  }

  /** Remove an event listener. */
  removeEventListener(subscription: AudioEventSubscription): void {
    subscription.remove();
  }
}

export default new RecordingNotificationManager();

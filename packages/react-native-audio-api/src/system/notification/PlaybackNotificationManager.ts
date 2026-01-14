import { AudioEventEmitter, AudioEventSubscription } from '../../events';
import { NativeAudioAPIModule } from '../../specs';
import type {
  NotificationEvents,
  NotificationManager,
  PlaybackControlName,
  PlaybackNotificationEventName,
  PlaybackNotificationInfo,
} from './types';
import { AudioApiError } from '../../errors';

/// Manager for media playback notifications with controls and MediaSession integration.
class PlaybackNotificationManager
  implements
    NotificationManager<PlaybackNotificationInfo, PlaybackNotificationEventName>
{
  private notificationKey = 'playback';
  private audioEventEmitter: AudioEventEmitter;

  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  /// Show the notification with metadata or update if already visible.
  /// Automatically creates the notification on first call.
  async show(info: PlaybackNotificationInfo): Promise<void> {
    if (!NativeAudioAPIModule) {
      throw new AudioApiError('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.showNotification(
      'playback',
      this.notificationKey,
      info as Record<string, string | number | boolean | undefined>
    );

    if (result.error) {
      throw new AudioApiError(result.error);
    }
  }

  /// Update the notification with new metadata or state.
  /// This is an alias for show() since show handles both initial display and updates.
  async update(info: PlaybackNotificationInfo): Promise<void> {
    return this.show(info);
  }

  /// Hide the notification.
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

  /// Enable or disable a specific playback control.
  async enableControl(
    control: PlaybackControlName,
    enabled: boolean
  ): Promise<void> {
    if (!NativeAudioAPIModule) {
      throw new AudioApiError('NativeAudioAPIModule is not available');
    }

    const params = { control, enabled };
    const result = await NativeAudioAPIModule.showNotification(
      'playback',
      this.notificationKey,
      params as Record<string, string | number | boolean | undefined>
    );

    if (result.error) {
      throw new AudioApiError(result.error);
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
  addEventListener<T extends PlaybackNotificationEventName>(
    eventName: T,
    callback: (event: NotificationEvents[T]) => void
  ): AudioEventSubscription {
    return this.audioEventEmitter.addAudioEventListener(eventName, callback);
  }
}

export default new PlaybackNotificationManager();

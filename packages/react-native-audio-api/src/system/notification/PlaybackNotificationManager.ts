import { NativeAudioAPIModule } from '../../specs';
import { AudioEventEmitter, AudioEventSubscription } from '../../events';
import type {
  NotificationManager,
  PlaybackNotificationInfo,
  PlaybackControlName,
  PlaybackNotificationEventName,
  NotificationEvents,
} from './types';

/// Manager for media playback notifications with controls and MediaSession integration.
class PlaybackNotificationManager
  implements
    NotificationManager<
      PlaybackNotificationInfo,
      PlaybackNotificationInfo,
      PlaybackNotificationEventName
    >
{
  private notificationKey = 'playback';
  private isRegistered = false;
  private isShown = false;
  private audioEventEmitter: AudioEventEmitter;

  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  /// Register the playback notification (must be called before showing).
  async register(): Promise<void> {
    if (this.isRegistered) {
      console.warn('PlaybackNotification is already registered');
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.registerNotification(
      'playback',
      this.notificationKey
    );

    if (result.error) {
      throw new Error(result.error);
    }

    this.isRegistered = true;
  }

  /// Show the notification with initial metadata.
  async show(info: PlaybackNotificationInfo): Promise<void> {
    if (!this.isRegistered) {
      throw new Error(
        'PlaybackNotification must be registered before showing. Call register() first.'
      );
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.showNotification(
      this.notificationKey,
      info as Record<string, string | number | boolean | undefined>
    );

    if (result.error) {
      throw new Error(result.error);
    }

    this.isShown = true;
  }

  /// Update the notification with new metadata or state.
  async update(info: PlaybackNotificationInfo): Promise<void> {
    if (!this.isShown) {
      console.warn('PlaybackNotification is not shown. Call show() first.');
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.updateNotification(
      this.notificationKey,
      info as Record<string, string | number | boolean | undefined>
    );

    if (result.error) {
      throw new Error(result.error);
    }
  }

  /// Hide the notification (can be shown again later).
  async hide(): Promise<void> {
    if (!this.isShown) {
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.hideNotification(
      this.notificationKey
    );

    if (result.error) {
      throw new Error(result.error);
    }

    this.isShown = false;
  }

  /// Unregister the notification (must register again to use).
  async unregister(): Promise<void> {
    if (!this.isRegistered) {
      return;
    }

    if (this.isShown) {
      await this.hide();
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.unregisterNotification(
      this.notificationKey
    );

    if (result.error) {
      throw new Error(result.error);
    }

    this.isRegistered = false;
  }

  /// Enable or disable a specific playback control.
  async enableControl(
    control: PlaybackControlName,
    enabled: boolean
  ): Promise<void> {
    if (!this.isRegistered) {
      console.warn('PlaybackNotification is not registered');
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const params = { control, enabled };
    const result = await NativeAudioAPIModule.updateNotification(
      this.notificationKey,
      params as Record<string, string | number | boolean | undefined>
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
  addEventListener<T extends PlaybackNotificationEventName>(
    eventName: T,
    callback: (event: NotificationEvents[T]) => void
  ): AudioEventSubscription {
    return this.audioEventEmitter.addAudioEventListener(eventName, callback);
  }

  /** Remove an event listener. */
  removeEventListener(subscription: AudioEventSubscription): void {
    subscription.remove();
  }
}

export default new PlaybackNotificationManager();

import { Platform } from 'react-native';
import { NativeAudioAPIModule } from '../../specs';
import { AudioEventEmitter, AudioEventSubscription } from '../../events';
import type {
  NotificationManager,
  RecordingNotificationInfo,
  RecordingControlName,
  RecordingNotificationEventName,
  NotificationEvents,
} from './types';

/// Manager for recording notifications with controls.
class RecordingNotificationManager
  implements
    NotificationManager<
      RecordingNotificationInfo,
      RecordingNotificationInfo,
      RecordingNotificationEventName
    >
{
  private notificationKey = 'recording';
  private isRegistered = false;
  private isShown = false;
  private audioEventEmitter: AudioEventEmitter;
  private isIOS = Platform.OS === 'ios';

  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  /// Register the recording notification (must be called before showing).
  async register(): Promise<void> {
    if (this.isRegistered) {
      console.warn('RecordingNotification is already registered');
      return;
    }

    // Recording notifications are only supported on Android
    if (this.isIOS) {
      this.isRegistered = true;
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.registerNotification(
      'recording',
      this.notificationKey
    );

    if (result.error) {
      throw new Error(result.error);
    }

    this.isRegistered = true;
  }

  /// Show the notification with initial metadata.
  async show(info: RecordingNotificationInfo): Promise<void> {
    if (!this.isRegistered) {
      throw new Error(
        'RecordingNotification must be registered before showing. Call register() first.'
      );
    }

    // Recording notifications are only supported on Android
    if (this.isIOS) {
      this.isShown = true;
      return;
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
  async update(info: RecordingNotificationInfo): Promise<void> {
    if (!this.isShown) {
      console.warn('RecordingNotification is not shown. Call show() first.');
      return;
    }

    // Recording notifications are only supported on Android
    if (this.isIOS) {
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

    // Recording notifications are only supported on Android
    if (this.isIOS) {
      this.isShown = false;
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

    // Recording notifications are only supported on Android
    if (this.isIOS) {
      this.isRegistered = false;
      return;
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

  /// Enable or disable a specific recording control.
  async enableControl(
    control: RecordingControlName,
    enabled: boolean
  ): Promise<void> {
    if (!this.isRegistered) {
      console.warn('RecordingNotification is not registered');
      return;
    }

    // Recording notifications are only supported on Android
    if (this.isIOS) {
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
    // Recording notifications are only supported on Android
    if (this.isIOS) {
      return this.isShown;
    }

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
    callback: (event: NotificationEvents[T]) => void
  ): AudioEventSubscription {
    // Recording notifications are only supported on Android
    if (this.isIOS) {
      // Return a dummy subscription for iOS
      return {
        remove: () => {},
      } as unknown as AudioEventSubscription;
    }

    return this.audioEventEmitter.addAudioEventListener(eventName, callback);
  }

  /** Remove an event listener. */
  removeEventListener(subscription: AudioEventSubscription): void {
    subscription.remove();
  }
}

export default new RecordingNotificationManager();

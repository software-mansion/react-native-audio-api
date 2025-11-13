import { NativeAudioAPIModule } from '../../specs';
import { AudioEventEmitter, AudioEventSubscription } from '../../events';
import type { NotificationManager, SimpleNotificationOptions } from './types';

/// Simple notification manager for basic notifications with title and text.
/// Implements the generic NotificationManager interface.
// It is only a showcase
class SimpleNotificationManager
  implements
    NotificationManager<
      SimpleNotificationOptions,
      SimpleNotificationOptions,
      never
    >
{
  private notificationKey = 'simple';
  private isRegistered = false;
  private isShown = false;
  private audioEventEmitter: AudioEventEmitter;

  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  /// Register the simple notification (must be called before showing).
  async register(): Promise<void> {
    if (this.isRegistered) {
      console.warn('SimpleNotification is already registered');
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.registerNotification(
      'simple',
      this.notificationKey
    );

    if (result.error) {
      throw new Error(result.error);
    }

    this.isRegistered = true;
  }

  /// Show the notification with initial options.
  async show(options: SimpleNotificationOptions): Promise<void> {
    if (!this.isRegistered) {
      throw new Error(
        'SimpleNotification must be registered before showing. Call register() first.'
      );
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.showNotification(
      this.notificationKey,
      options as Record<string, string | number | boolean | undefined>
    );

    if (result.error) {
      throw new Error(result.error);
    }

    this.isShown = true;
  }

  /// Update the notification with new options.
  async update(options: SimpleNotificationOptions): Promise<void> {
    if (!this.isShown) {
      console.warn('SimpleNotification is not shown. Call show() first.');
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new Error('NativeAudioAPIModule is not available');
    }

    const result = await NativeAudioAPIModule.updateNotification(
      this.notificationKey,
      options as Record<string, string | number | boolean | undefined>
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

  /// Check if the notification is currently active.
  async isActive(): Promise<boolean> {
    if (!NativeAudioAPIModule) {
      return false;
    }

    return await NativeAudioAPIModule.isNotificationActive(
      this.notificationKey
    );
  }

  /// Add an event listener (SimpleNotification doesn't emit events).
  addEventListener<T extends never>(
    _eventName: T,
    _callback: (event: never) => void
  ): AudioEventSubscription {
    // SimpleNotification doesn't emit events, return a no-op subscription
    console.warn('SimpleNotification does not support event listeners');
    return this.audioEventEmitter.addAudioEventListener(
      // Using a valid event name for the no-op subscription
      'playbackNotificationPlay',
      () => {}
    );
  }

  /// Remove an event listener.
  removeEventListener(subscription: AudioEventSubscription): void {
    subscription.remove();
  }
}

export default new SimpleNotificationManager();

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

class PlaybackNotificationManager
  implements
    NotificationManager<PlaybackNotificationInfo, PlaybackNotificationEventName>
{
  private notificationKey = 'playback';
  private audioEventEmitter: AudioEventEmitter;

  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  /**
   * Show the notification with metadata or update if already visible.
   * Automatically creates the notification on first call.
   *
   * @param info - The info to be displayed.
   * @returns Promise that resolves after creating notification.
   */
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
   * Enable or disable a specific playback control.
   *
   * @param control - The control to enable or disable on the notification.
   * @param enabled - Whether to enable (true) or disable (false) the control.
   * @returns Promise that resolves after showing modified notification.
   */
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
  addEventListener<T extends PlaybackNotificationEventName>(
    eventName: T,
    callback: (event: NotificationEvents[T]) => void
  ): AudioEventSubscription {
    return this.audioEventEmitter.addAudioEventListener(eventName, callback);
  }
}

export default new PlaybackNotificationManager();

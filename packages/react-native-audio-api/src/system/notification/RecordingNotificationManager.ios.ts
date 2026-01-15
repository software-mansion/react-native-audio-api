import { AudioEventEmitter, AudioEventSubscription } from '../../events';
import type {
  NotificationManager,
  RecordingNotificationEvent,
  RecordingNotificationEventName,
  RecordingNotificationInfo,
} from './types';

class RecordingNotificationManager
  implements
    NotificationManager<
      RecordingNotificationInfo,
      RecordingNotificationEventName
    >
{
  private audioEventEmitter: AudioEventEmitter;

  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
    console.warn(
      'RecordingNotificationManager is not implemented on iOS. Any calls to it will be no-ops.'
    );
  }

  /**
   * Show the notification with metadata or update if already visible.
   *
   * @param info - The info to be displayed.
   * @returns Promise that resolves after creating notification.
   */
  async show(_info: RecordingNotificationInfo): Promise<void> {}

  /**
   * Hide the notification.
   *
   * @returns Promise that resolves after hiding notification.
   */
  async hide(): Promise<void> {}

  /**
   * Check if the notification is currently active.
   *
   * @returns Promise that resolves to whether notification is active.
   */
  // eslint-disable-next-line @typescript-eslint/require-await
  async isActive(): Promise<boolean> {
    return false;
  }

  /**
   * Add an event listener for notification actions.
   *
   * @param eventName - The event name to listen for.
   * @param callback - The callback to invoke on event.
   * @returns Promise that resolves to whether notification is active.
   */
  addEventListener<T extends RecordingNotificationEventName>(
    eventName: T,
    callback: (event: RecordingNotificationEvent[T]) => void
  ): AudioEventSubscription {
    return this.audioEventEmitter.addAudioEventListener(eventName, callback);
  }
}

export default new RecordingNotificationManager();

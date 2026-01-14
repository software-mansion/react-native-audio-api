/* eslint-disable @typescript-eslint/no-unused-vars */
/* eslint-disable no-useless-constructor */
/* eslint-disable @typescript-eslint/require-await */

import type { AudioEventSubscription } from '../../events';
import type {
  NotificationEvents,
  NotificationManager,
  RecordingNotificationEventName,
  RecordingNotificationInfo,
} from '../../system';

/// Mock Manager for playback notifications. Does nothing.
class RecordingNotificationManager
  implements
    NotificationManager<
      RecordingNotificationInfo,
      RecordingNotificationEventName
    >
{
  private isRegistered_ = false;
  private isShown_ = false;

  constructor() {}

  async show(info: RecordingNotificationInfo): Promise<void> {}

  async hide(): Promise<void> {}

  async isActive(): Promise<boolean> {
    return this.isShown_;
  }

  isRegistered(): boolean {
    return this.isRegistered_;
  }

  addEventListener<T extends RecordingNotificationEventName>(
    eventName: T,
    callback: (event: NotificationEvents[T]) => void
  ): AudioEventSubscription {
    // dummy subscription object with a no-op remove method
    return {
      remove: () => {},
    } as unknown as AudioEventSubscription;
  }
}

export default new RecordingNotificationManager();

/* eslint-disable @typescript-eslint/no-unused-vars */
/* eslint-disable no-useless-constructor */
/* eslint-disable @typescript-eslint/require-await */

import type { AudioEventSubscription } from '../../events';
import type {
  NotificationEvents,
  NotificationManager,
  PlaybackControlName,
  PlaybackNotificationEventName,
  PlaybackNotificationInfo,
} from '../../system';

/// Mock Manager for playback notifications. Does nothing.
class PlaybackNotificationManager
  implements
    NotificationManager<PlaybackNotificationInfo, PlaybackNotificationEventName>
{
  private isRegistered_ = false;
  private isShown_ = false;

  constructor() {}

  async register(): Promise<void> {}

  async show(info: PlaybackNotificationInfo): Promise<void> {}

  async update(info: PlaybackNotificationInfo): Promise<void> {}

  async hide(): Promise<void> {}

  async unregister(): Promise<void> {}

  async enableControl(
    control: PlaybackControlName,
    enabled: boolean
  ): Promise<void> {}

  async isActive(): Promise<boolean> {
    return this.isShown_;
  }

  isRegistered(): boolean {
    return this.isRegistered_;
  }

  addEventListener<T extends PlaybackNotificationEventName>(
    eventName: T,
    callback: (event: NotificationEvents[T]) => void
  ): AudioEventSubscription {
    // dummy subscription object with a no-op remove method
    return {
      remove: () => {},
    } as unknown as AudioEventSubscription;
  }
}

export default new PlaybackNotificationManager();

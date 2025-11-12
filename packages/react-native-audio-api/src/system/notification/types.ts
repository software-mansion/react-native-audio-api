import type { AudioEventSubscription } from '../../events';

/// Generic notification manager interface that all notification managers should implement.
/// Provides a consistent API for managing notification lifecycle and events.
export interface NotificationManager<
  TShowOptions,
  TUpdateOptions,
  TEventName extends string = string,
> {
  /// Register the notification (must be called before showing).
  register(): Promise<void>;

  /// Show the notification with initial options.
  show(options: TShowOptions): Promise<void>;

  /// Update the notification with new options.
  update(options: TUpdateOptions): Promise<void>;

  /// Hide the notification (can be shown again later).
  hide(): Promise<void>;

  /// Unregister the notification (must register again to use).
  unregister(): Promise<void>;

  /// Check if the notification is currently active.
  isActive(): Promise<boolean>;

  /// Add an event listener for notification events.
  addEventListener<T extends TEventName>(
    eventName: T,
    callback: (event: unknown) => void
  ): AudioEventSubscription;

  /// Remove an event listener.
  removeEventListener(subscription: AudioEventSubscription): void;
}

/// Metadata and state information for playback notifications.
export interface PlaybackNotificationInfo {
  title?: string;
  artist?: string;
  album?: string;
  artwork?: string | { uri: string };
  duration?: number;
  elapsedTime?: number;
  speed?: number;
  state?: 'playing' | 'paused';
}

/// Available playback control actions.
export type PlaybackControlName =
  | 'play'
  | 'pause'
  | 'next'
  | 'previous'
  | 'skipForward'
  | 'skipBackward';

/// Event names for playback notification actions.
export type PlaybackNotificationEventName =
  | 'playbackNotificationPlay'
  | 'playbackNotificationPause'
  | 'playbackNotificationNext'
  | 'playbackNotificationPrevious'
  | 'playbackNotificationSkipForward'
  | 'playbackNotificationSkipBackward'
  | 'playbackNotificationDismissed';

/// Options for a simple notification with title and text.
export interface SimpleNotificationOptions {
  title?: string;
  text?: string;
}

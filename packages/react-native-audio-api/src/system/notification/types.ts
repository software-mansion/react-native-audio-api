import type { AudioEventSubscription } from '../../events';
import { EventEmptyType, EventTypeWithValue } from '../../events/types';

/// Generic notification manager interface that all notification managers should implement.
/// Provides a consistent API for managing notification lifecycle and events.
export interface NotificationManager<
  TShowOptions,
  TUpdateOptions,
  TEventName extends NotificationEventName,
> {
  /// Show the notification with options or update if already visible.
  /// Automatically creates the notification instance on first call.
  show(options: TShowOptions): Promise<void>;

  /// Update the notification with new options (alias for show).
  update(options: TUpdateOptions): Promise<void>;

  /// Hide the notification.
  hide(): Promise<void>;

  /// Check if the notification is currently active.
  isActive(): Promise<boolean>;

  /// Add an event listener for notification events.
  addEventListener<T extends TEventName>(
    eventName: T,
    callback: NotificationCallback<T>
  ): AudioEventSubscription | undefined;

  /// Remove an event listener.
  removeEventListener(subscription: AudioEventSubscription): void;
}

/// Metadata and state information for playback notifications.
export interface PlaybackNotificationInfo {
  title?: string;
  artist?: string;
  album?: string;
  artwork?: string | { uri: string };
  androidSmallIcon?: string | { uri: string };
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
  | 'skipBackward'
  | 'seekTo';

/// Event names for playback notification actions.
interface PlaybackNotificationEvent {
  playbackNotificationPlay: EventEmptyType;
  playbackNotificationPause: EventEmptyType;
  playbackNotificationNext: EventEmptyType;
  playbackNotificationPrevious: EventEmptyType;
  playbackNotificationSkipForward: EventTypeWithValue;
  playbackNotificationSkipBackward: EventTypeWithValue;
  playbackNotificationSeekTo: EventTypeWithValue;
  playbackNotificationDismissed: EventEmptyType;
}

export type PlaybackNotificationEventName = keyof PlaybackNotificationEvent;

export type NotificationEvents = PlaybackNotificationEvent;

export type NotificationEventName = keyof NotificationEvents;

export type NotificationCallback<Name extends NotificationEventName> = (
  event: NotificationEvents[Name]
) => void;

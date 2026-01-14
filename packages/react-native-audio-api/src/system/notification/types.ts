import type { AudioEventSubscription } from '../../events';
import { EventEmptyType, EventTypeWithValue } from '../../events/types';

/// Generic notification manager interface that all notification managers should implement.
/// Provides a consistent API for managing notification lifecycle and events.
export interface NotificationManager<
  TShowOptions,
  TEventName extends NotificationEventName,
> {
  /// Show the notification with options or update if already visible.
  /// Automatically creates the notification instance on first call.
  show(options: TShowOptions): Promise<void>;

  /// Hide the notification.
  hide(): Promise<void>;

  /// Check if the notification is currently active.
  isActive(): Promise<boolean>;

  /// Add an event listener for notification events.
  addEventListener<T extends TEventName>(
    eventName: T,
    callback: NotificationCallback<T>
  ): AudioEventSubscription | undefined;
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

export interface RecordingNotificationInfo {
  title?: string;
  contentText?: string;
  paused?: boolean;
  smallIconResourceName?: string;
  largeIconResourceName?: string;
  pauseIconResourceName?: string;
  resumeIconResourceName?: string;
  color?: number;
}

export interface RecordingNotificationEvent {
  recordingNotificationPause: EventEmptyType;
  recordingNotificationResume: EventEmptyType;
}

export type PlaybackNotificationEventName = keyof PlaybackNotificationEvent;

export type RecordingNotificationEventName = keyof RecordingNotificationEvent;

export type NotificationEvents = PlaybackNotificationEvent &
  RecordingNotificationEvent;

export type NotificationEventName = keyof NotificationEvents;

export type NotificationCallback<Name extends NotificationEventName> = (
  event: NotificationEvents[Name]
) => void;

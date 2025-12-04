import type { AudioEventSubscription } from '../../events';
import { EventEmptyType, EventTypeWithValue } from '../../events/types';

/// Generic notification manager interface that all notification managers should implement.
/// Provides a consistent API for managing notification lifecycle and events.
export interface NotificationManager<
  TShowOptions,
  TUpdateOptions,
  TEventName extends NotificationEventName,
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
    callback: NotificationCallback<T>
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

/// Metadata and state information for recording notifications.
export interface RecordingNotificationInfo {
  title?: string;
  description?: string;
  artwork?: string | { uri: string };
  state?: 'recording' | 'stopped';
  control?: RecordingControlName;
  enabled?: boolean;
}

/// Available recording control actions.
export type RecordingControlName = 'start' | 'stop';

/// Event names for recording notification actions.
interface RecordingNotificationEvent {
  recordingNotificationStart: EventEmptyType;
  recordingNotificationStop: EventEmptyType;
  recordingNotificationDismissed: EventEmptyType;
}

export type RecordingNotificationEventName = keyof RecordingNotificationEvent;

export type NotificationEvents = PlaybackNotificationEvent &
  RecordingNotificationEvent;
export type NotificationEventName = keyof NotificationEvents;

export type NotificationCallback<Name extends NotificationEventName> = (
  event: NotificationEvents[Name]
) => void;

/// Options for a simple notification with title and text.
export interface SimpleNotificationOptions {
  title?: string;
  text?: string;
}

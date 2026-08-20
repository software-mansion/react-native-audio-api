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
  /** Skip interval in seconds for skipForward/skipBackward. Default: 15 */
  skipInterval?: number;
}

/// Available playback control actions.
export type PlaybackControlName =
  | 'play'
  | 'pause'
  | 'stop'
  | 'nextTrack'
  | 'previousTrack'
  | 'skipForward'
  | 'skipBackward'
  | 'seekTo';

/// Event names for playback notification actions.
interface PlaybackNotificationEvent {
  playbackNotificationPlay: EventEmptyType;
  playbackNotificationPause: EventEmptyType;
  playbackNotificationStop: EventEmptyType;
  playbackNotificationNextTrack: EventEmptyType;
  playbackNotificationPreviousTrack: EventEmptyType;
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
  color?: number;
  /**
   * Shows a stop action that ends the recording natively — it works even when
   * the app task has been removed and JS is unreachable. A live app is
   * additionally notified through the `recordingNotificationStop` event.
   * Default: false.
   */
  showStopAction?: boolean;
  /** Label of the pause action. Default: 'Pause'. */
  pauseActionTitle?: string;
  /** Label of the resume action. Default: 'Resume'. */
  resumeActionTitle?: string;
  /** Label of the stop action. Default: 'Stop'. */
  stopActionTitle?: string;
  /**
   * URI attached to the notification tap intent, e.g. `myapp://record`.
   * Delivered through React Native's `Linking` (initial URL on cold start,
   * `url` event otherwise), so it can route to a specific screen. Without it,
   * tapping the notification opens the app's launcher activity.
   */
  deepLinkUri?: string;
  /** Shows the elapsed recording time in the notification. Default: false. */
  usesChronometer?: boolean;
}

export interface RecordingNotificationEvent {
  recordingNotificationPause: EventEmptyType;
  recordingNotificationResume: EventEmptyType;
  recordingNotificationStop: EventEmptyType;
}

export type PlaybackNotificationEventName = keyof PlaybackNotificationEvent;

export type RecordingNotificationEventName = keyof RecordingNotificationEvent;

export type NotificationEvents = PlaybackNotificationEvent &
  RecordingNotificationEvent;

export type NotificationEventName = keyof NotificationEvents;

export type NotificationCallback<Name extends NotificationEventName> = (
  event: NotificationEvents[Name]
) => void;

import AudioBuffer from '../core/AudioBuffer';
import { NotificationEvents } from '../system';

export interface EventEmptyType {}

export interface EventTypeWithValue {
  value: number;
}

export interface EventTypeWithBool {
  value: boolean;
}

export type InterruptionType = 'began' | 'ended';

export interface OnInterruptionEventType {
  type: InterruptionType;
  shouldResume: boolean;
}

export type RouteChangeReason =
  | 'Unknown'
  | 'Override'
  | 'CategoryChange'
  | 'WakeFromSleep'
  | 'NewDeviceAvailable'
  | 'OldDeviceUnavailable'
  | 'ConfigurationChange'
  | 'NoSuitableRouteForCategory';

export interface OnRouteChangeEventType {
  reason: RouteChangeReason;
}

export interface OnRecorderErrorEventType {
  message: string;
}

type SystemEvents = {
  volumeChange: EventTypeWithValue;
  interruption: OnInterruptionEventType;
  duck: EventEmptyType;
  routeChange: OnRouteChangeEventType;
};

export interface OnBufferEndEventType {
  bufferId: string;
  isLastBufferInQueue: boolean;
}

/**
 * Represents the data payload received by the audio recorder callback each time
 * a new audio buffer becomes available during recording.
 */
export interface OnAudioReadyEventType {
  /**
   * The audio buffer containing the recorded PCM data. This buffer includes one
   * or more channels of floating-point samples in the range of -1.0 to 1.0.
   */
  buffer: AudioBuffer;

  /**
   * The number of audio frames contained in this buffer. A frame represents a
   * single sample across all channels.
   */
  numFrames: number;

  /**
   * The timestamp (in seconds) indicating when this buffer was captured,
   * relative to the start of the recording session.
   */
  when: number;
}

interface AudioAPIEvents {
  ended: EventEmptyType;
  loopEnded: EventEmptyType;
  audioReady: OnAudioReadyEventType;
  positionChanged: EventTypeWithValue;
  bufferEnded: OnBufferEndEventType;
  recorderError: OnRecorderErrorEventType;
  /**
   * Fires when an `<Audio>` source starts or stops stalling on decoded data — a
   * real network/decoder stall (debounced natively against normal decode-ahead
   * jitter), not a deliberate pause. `value` is true while stalled. See
   * `AudioTagPlaybackState`'s `'buffering'` state and the
   * `onWaiting`/`onPlaying` props on `<Audio>`.
   */
  bufferingStateChanged: EventTypeWithBool;
}

type AudioEvents = SystemEvents & AudioAPIEvents & NotificationEvents;

export type SystemEventName = keyof SystemEvents;
export type SystemEventCallback<Name extends SystemEventName> = (
  event: SystemEvents[Name]
) => void;

export type AudioAPIEventName = keyof AudioAPIEvents;
export type AudioAPIEventCallback<Name extends AudioAPIEventName> = (
  event: AudioAPIEvents[Name]
) => void;

export type AudioEventName = keyof AudioEvents;
export type AudioEventCallback<Name extends AudioEventName> = (
  event: AudioEvents[Name]
) => void;

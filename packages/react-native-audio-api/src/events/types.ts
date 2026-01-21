import AudioBuffer from '../core/AudioBuffer';
import { NotificationEvents } from '../system';

export interface EventEmptyType {}

export interface EventTypeWithValue {
  value: number;
}

interface OnInterruptionEventType {
  type: 'ended' | 'began';
  shouldResume: boolean;
}

interface OnRouteChangeEventType {
  reason:
    | 'Unknown'
    | 'Override'
    | 'CategoryChange'
    | 'WakeFromSleep'
    | 'NewDeviceAvailable'
    | 'OldDeviceUnavailable'
    | 'ConfigurationChange'
    | 'NoSuitableRouteForCategory';
}

export interface OnRecorderErrorEventType {
  message: string;
}

interface RemoteCommandEvents {
  remotePlay: EventEmptyType;
  remotePause: EventEmptyType;
  remoteStop: EventEmptyType;
  remoteTogglePlayPause: EventEmptyType;
  remoteChangePlaybackRate: EventTypeWithValue;
  remoteNextTrack: EventEmptyType;
  remotePreviousTrack: EventEmptyType;
  remoteSkipForward: EventTypeWithValue;
  remoteSkipBackward: EventTypeWithValue;
  remoteSeekForward: EventEmptyType;
  remoteSeekBackward: EventEmptyType;
  remoteChangePlaybackPosition: EventTypeWithValue;
}

type SystemEvents = RemoteCommandEvents & {
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
  audioError: EventEmptyType; // to change
  systemStateChanged: EventEmptyType; // to change
  recorderError: OnRecorderErrorEventType;
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

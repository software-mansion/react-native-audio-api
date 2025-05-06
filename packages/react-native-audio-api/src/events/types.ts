interface EventEmptyType {}

export interface EventTypeWithValue {
  value: number;
}

interface OnInterruptionEventType {
  type: 'ended' | 'began';
  shouldResume: boolean;
}

interface OnRouteChangeEventType {
  reason: string;
}

interface SystemEvents {
  remotePlay: EventEmptyType;
  remotePause: EventEmptyType;
  remoteStop: EventEmptyType;
  remoteTogglePlayPause: EventEmptyType;
  remoteChangePlaybackRate: EventTypeWithValue;
  remoteNextTrack: EventEmptyType;
  remotePreviousTrack: EventEmptyType;
  remoteSkipForward: EventEmptyType;
  remoteSkipBackward: EventEmptyType;
  remoteSeekForward: EventTypeWithValue;
  remoteSeekBackward: EventTypeWithValue;
  remoteChangePlaybackPosition: EventTypeWithValue;
  volumeChange: EventTypeWithValue;
  interruption: OnInterruptionEventType;
  routeChange: OnRouteChangeEventType;
}

interface AudioAPIEvents {
  ended: EventTypeWithValue;
  audioReady: EventEmptyType; // to change
  audioError: EventEmptyType; // to change
  systemStateChanged: EventEmptyType; // to change
}

type AudioEvents = SystemEvents & AudioAPIEvents;

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

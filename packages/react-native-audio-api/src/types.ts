export type ChannelCountMode = 'max' | 'clamped-max' | 'explicit';

export type ChannelInterpretation = 'speakers' | 'discrete';

export type BiquadFilterType =
  | 'lowpass'
  | 'highpass'
  | 'bandpass'
  | 'lowshelf'
  | 'highshelf'
  | 'peaking'
  | 'notch'
  | 'allpass';

export type ContextState = 'running' | 'closed';

export type OscillatorType =
  | 'sine'
  | 'square'
  | 'sawtooth'
  | 'triangle'
  | 'custom';

export interface PeriodicWaveConstraints {
  disableNormalization: boolean;
}

export type WindowType = 'blackman' | 'hann';

export type IOSCategory =
  | 'record'
  | 'ambient'
  | 'playback'
  | 'multiRoute'
  | 'soloAmbient'
  | 'playbackAndRecord';

export type IOSMode =
  | 'default'
  | 'gameChat'
  | 'videoChat'
  | 'voiceChat'
  | 'measurement'
  | 'voicePrompt'
  | 'spokenAudio'
  | 'moviePlayback'
  | 'videoRecording';

export type IOSCategoryOption =
  | 'duckOthers'
  | 'allowAirPlay'
  | 'mixWithOthers'
  | 'allowBluetooth'
  | 'defaultToSpeaker'
  | 'allowBluetoothA2DP'
  | 'overrideMutedMicrophoneInterruption'
  | 'interruptSpokenAudioAndMixWithOthers';

export type InterruptionMode = 'manual' | 'disabled' | 'automatic';

export type AudioEventName =
  | 'play'
  | 'pause'
  | 'stop'
  | 'ended'
  | 'nextTrack'
  | 'previousTrack'
  | 'changePosition'
  | 'seek'
  | 'seekForward'
  | 'seekBackward'
  | 'skipForward'
  | 'skipBackward'
  | 'androidClose'
  | 'androidSetVolume'
  | 'androidSetRating'
  | 'iosTogglePlayPause'
  | 'iosEnableLanguageOption'
  | 'iosDisableLanguageOption';

export interface AudioSessionOptions {
  iosMode?: IOSMode;
  iosCategory?: IOSCategory;
  androidForegroundService?: boolean;
  interruptionMode?: InterruptionMode;
  iosCategoryOptions?: IOSCategoryOption[];
}

interface ChangePositionEventData {
  position: number;
}

interface SeekEventData {
  position: number;
}

interface SetVolumeAndroidEventData {
  volume: number;
}

interface SetRatingAndroidEventData {
  heart: boolean;
  thumbsUp: boolean;
  value?: number;
}

interface EventData extends Record<AudioEventName, unknown> {
  play: undefined;
  stop: undefined;
  pause: undefined;
  nextTrack: undefined;
  previousTrack: undefined;
  changePosition: ChangePositionEventData;
  seek: SeekEventData;
  seekForward: undefined;
  seekBackward: undefined;
  skipForward: undefined;
  skipBackward: undefined;
  androidClose: undefined;
  androidSetVolume: SetVolumeAndroidEventData;
  androidSetRating: SetRatingAndroidEventData;
  iosTogglePlayPause: undefined;
  iosEnableLanguageOption: undefined;
  iosDisableLanguageOption: undefined;
}

export interface AudioEventPayload<EN extends AudioEventName> {
  name: EN;
  data: EventData[EN];
}

export type AudioEventCallback<EN extends AudioEventName> = (
  event: AudioEventPayload<EN>
) => void | Promise<void>;

export interface EventListener {
  remove: () => void;
}

export interface NowPlayingInfo {
  title?: string;
  artist?: string;
  artwork?: string;
  duration?: number;
}

export interface NowPlayingInfoUpdate extends NowPlayingInfo {
  isPlaying: boolean;
  elapsedTime: number;
}

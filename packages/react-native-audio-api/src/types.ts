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

export enum IOSCategory {
  Record = 0,
  Ambient = 1,
  Playback = 2,
  MultiRoute = 3,
  SoloAmbient = 4,
  PlaybackAndRecord = 5,
}

export enum IOSMode {
  Default = 0,
  GameChat = 1,
  VideoChat = 2,
  VoiceChat = 3,
  Measurement = 4,
  VoicePrompt = 5,
  SpokenAudio = 6,
  MoviePlayback = 7,
  VideoRecording = 8,
}

export enum IOSCategoryOption {
  DuckOthers = 0,
  AllowAirPlay = 1,
  MixWithOthers = 2,
  AllowBluetooth = 3,
  DefaultToSpeaker = 4,
  AllowBluetoothA2DP = 5,
  OverrideMutedMicrophoneInterruption = 6,
  InterruptSpokenAudioAndMixWithOthers = 7,
}

export enum InterruptionMode {
  Manual = 0,
  Disabled = 1,
  Automatic = 2,
}

export interface AudioSessionOptions {
  iosMode?: IOSMode;
  iosCategory?: IOSCategory;
  androidForegroundService?: boolean;
  interruptionMode?: InterruptionMode;
  iosCategoryOptions?: IOSCategoryOption[];
}

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

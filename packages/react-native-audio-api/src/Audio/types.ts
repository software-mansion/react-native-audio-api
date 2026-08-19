import { ReactNode } from 'react';
import type BaseAudioContext from '../core/BaseAudioContext';
import type { IAudioFileSourceNode } from '../jsi-interfaces';

export interface AudioURISource {
  uri?: string | undefined;
  // bundle?: string | undefined;
  // method?: string | undefined;
  headers?: { [key: string]: string } | undefined;
  // cache?: 'default' | 'reload' | 'force-cache' | 'only-if-cached' | undefined;
  // body?: string | undefined;
}

export type AudioRequireSource = number;

export interface TimeRanges {
  length: number;
  start(index: number): number;
  end(index: number): number;
}

export type AudioSource = AudioURISource | AudioRequireSource | string;

export type PreloadType = 'auto' | 'metadata' | 'none';

/**
 * `'buffering'` is a sub-state of `'playing'`: playback was started and hasn't
 * been paused/stopped, but the render thread is currently stalled waiting on
 * decoded data (a real network/decoder stall, debounced natively against normal
 * decode-ahead jitter — see `onWaiting`/`onPlaying`).
 */
export type AudioTagPlaybackState = 'idle' | 'playing' | 'paused' | 'buffering';

export interface AudioTagHandle {
  play: () => void;
  pause: () => void;
  seekToTime: (seconds: number) => void;
  setVolume: (volume: number) => void;
  setMuted: (muted: boolean) => void;
  setPlaybackRate: (playbackRate: number) => void;
}

/**
 * Internal handle surface used by MediaElementAudioSourceNode to obtain the
 * underlying file source. Not exported from the package public API.
 */
export interface InternalAudioTagHandle extends AudioTagHandle {
  getFileSourceNode: () => IAudioFileSourceNode | null;
}

interface AudioControlProps {
  autoPlay: boolean;
  controls: boolean; // TBD: should we support control display at all?
  loop: boolean;
  muted: boolean;
  preload: PreloadType;
  /**
   * When true, download the full remote file instead of streaming via HTTP
   * ranges. Native only — ignored on web.
   */
  forceDownload: boolean;
  source: AudioSource;
  playbackRate: number;
  preservesPitch: boolean;
  volume: number;
  children?: ReactNode;
  context?: BaseAudioContext; // optional on web, since web do not use AudioContext for audio tag
}

interface AudioReadonlyProps {
  // TODO: decide if we want to expose them this way
  // duration: number;
  // currentTime: number;
  // ended: boolean;
  // paused: boolean;
  // buffered: TimeRanges;
}

type TMPEmptyEventHandler = () => void;
type TMPNumberEventHandler = (number: number) => void;
type TMPErrorEventHandler = (error: Error) => void;

interface AudioEventProps {
  onLoadStart: TMPEmptyEventHandler;
  onLoad: TMPEmptyEventHandler;
  onError: TMPErrorEventHandler;
  onPositionChange: TMPNumberEventHandler;
  onEnded: TMPEmptyEventHandler;
  onPlay: TMPEmptyEventHandler;
  onPause: TMPEmptyEventHandler;
  onVolumeChange: TMPNumberEventHandler;
  /**
   * Fires when playback stalls waiting on decoded data — mirrors the HTML
   * `<audio>`/`<video>` `waiting` event. Not fired for a deliberate pause/seek,
   * only for a genuine stall while `playbackState === 'playing'`.
   */
  onWaiting: TMPEmptyEventHandler;
  /**
   * Fires when playback resumes after a stall reported via `onWaiting` —
   * mirrors the HTML `<audio>`/`<video>` `playing` event. Not fired for the
   * initial `play()` call — see `onPlay` for that.
   */
  onPlaying: TMPEmptyEventHandler;
}

export interface AudioPropsBase
  extends AudioControlProps, AudioReadonlyProps, AudioEventProps {}

export type AudioProps = Partial<AudioPropsBase> & { source: AudioSource };

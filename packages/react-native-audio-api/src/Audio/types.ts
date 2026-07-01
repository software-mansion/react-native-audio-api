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

export type AudioTagPlaybackState = 'idle' | 'playing' | 'paused';

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
}

export interface AudioPropsBase
  extends AudioControlProps, AudioReadonlyProps, AudioEventProps {}

export type AudioProps = Partial<AudioPropsBase> & { source: AudioSource };

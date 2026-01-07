export interface AudioURISource {
  uri?: string | undefined;
  // bundle?: string | undefined;
  method?: string | undefined;
  headers?: { [key: string]: string } | undefined;
  // cache?: 'default' | 'reload' | 'force-cache' | 'only-if-cached' | undefined;
  body?: string | undefined;
}

export type AudioRequireSource = number;

export interface TimeRanges {
  length: number;
  start(index: number): number;
  end(index: number): number;
}

export type AudioSource =
  | AudioURISource
  | AudioRequireSource
  | ReadonlyArray<AudioURISource>;

export type PreloadType = 'auto' | 'metadata' | 'none';

interface AudioControlProps {
  autoPlay: boolean;
  controls: boolean; // TBD: should we support control display at all?
  loop: boolean;
  muted: boolean;
  preload: PreloadType;
  source: AudioSource;
  playbackRate: number;
  preservesPitch: boolean;
  volume: number;
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

interface AudioEventProps {
  onLoadStart?: TMPEmptyEventHandler;
  onLoad?: TMPEmptyEventHandler;
  onError?: TMPEmptyEventHandler;
  onProgress?: TMPEmptyEventHandler;
  onSeeked?: TMPEmptyEventHandler;
  onEnded?: TMPEmptyEventHandler;
  onPlay?: TMPEmptyEventHandler;
  onPause?: TMPEmptyEventHandler;
}

export interface AudioPropsBase
  extends AudioControlProps,
    AudioReadonlyProps,
    AudioEventProps {}

export type AudioProps = Partial<AudioPropsBase> & { source: AudioSource };

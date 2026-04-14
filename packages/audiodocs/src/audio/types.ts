export interface BufferMetadata {
  id: string;
  duration: number;
}

export interface LoadedBuffer {
  id: string;
  buffer: AudioBuffer;
}

export type AudioManagerEventType = 'playing' | 'stopped' | 'outputVolumeChanged';

interface AudioManagerEventEmptyPayload {}

interface AudioManagerOutputVolumeChangedPayload {
  volume: number; // new volume level
}

export interface AudioManagerUpdatePayload {
  progress: number;
}

export type AudioManagerEventPayloads = {
  playing: AudioManagerEventEmptyPayload;
  stopped: AudioManagerEventEmptyPayload;
  outputVolumeChanged: AudioManagerOutputVolumeChangedPayload;
};

export interface AudioManagerEvent<T extends AudioManagerEventType = AudioManagerEventType> {
  type: T;
  payload: AudioManagerEventPayloads[T];
}

export interface ActiveSound {
  id: string;
  bufferId: string;
  startedAt: number;
  sourceNode: AudioBufferSourceNode;
}

export interface PlaySoundOptions {
  loop?: boolean;
  startFrom?: number;
  unique?: boolean;
  volume?: number;
  onEnded?: () => void;
}


export type PlaySoundReturnValue = false | { id: string, startedAt: number }

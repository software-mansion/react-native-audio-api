export type AudioMode = 'player' | 'amplifier';

export type AudioSource = 'music' | 'speech' | 'bgm' | 'efx';

export type SourceRecord<T> = Record<AudioSource, T>;

export interface AudioSourceMetadata {
  id: string;
  sourceName: AudioSource;
}

export interface SoundButtonProps {
  id: string;
  name: string;
  type: AudioSource;
  isActive?: boolean;
  onPlaySound?: (id: string, source: AudioSource) => void;
}


export type DisplayType = 'spectrogram' | 'equalizer';

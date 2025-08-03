import { AudioSource, AudioSourceMetadata, SourceRecord } from './types';

export const showAmpToolbarThreshold = 30;

export const soundSources: SourceRecord<string> = {
  music: '/react-native-audio-api/audio/music/example-music-05.mp3',
  speech: '/react-native-audio-api/audio/voice/voice-sample-landing.mp3',
  bgm: '/react-native-audio-api/audio/bgm/bgm-01.mp3',
  efx: '/react-native-audio-api/audio/efx/efx-01.mp3',
};

export const initialSounds: SourceRecord<AudioSourceMetadata | null> = {
  music: null,
  speech: null,
  bgm: null,
  efx: null,
};

export const labels: Record<AudioSource, string> = {
  music: 'Music',
  speech: 'Speech',
  bgm: 'BGM',
  efx: 'Effects'
};

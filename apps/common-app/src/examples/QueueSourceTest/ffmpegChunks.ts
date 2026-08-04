// TEMPORARY — assets produced by ./cut-chunks.sh (ffmpeg segment, ~1s each).
// Metro needs static requires, so list every chunk explicitly.

/* eslint-disable @typescript-eslint/no-var-requires */

export type SampleKey = 'pad' | 'musicCont' | 'tone';

export const FFMPEG_CHUNK_SECONDS = 1;

export const FFMPEG_CHUNKS: Record<
  SampleKey,
  { label: string; hint: string; assets: number[] }
> = {
  pad: {
    label: 'Continuous pad',
    hint: 'ffmpeg-segmented ~1s WAV chunks',
    assets: [
      require('./chunks/pad/chunk-000.wav') as number,
      require('./chunks/pad/chunk-001.wav') as number,
      require('./chunks/pad/chunk-002.wav') as number,
      require('./chunks/pad/chunk-003.wav') as number,
      require('./chunks/pad/chunk-004.wav') as number,
      require('./chunks/pad/chunk-005.wav') as number,
      require('./chunks/pad/chunk-006.wav') as number,
      require('./chunks/pad/chunk-007.wav') as number,
      require('./chunks/pad/chunk-008.wav') as number,
      require('./chunks/pad/chunk-009.wav') as number,
      require('./chunks/pad/chunk-010.wav') as number,
      require('./chunks/pad/chunk-011.wav') as number,
      require('./chunks/pad/chunk-012.wav') as number,
      require('./chunks/pad/chunk-013.wav') as number,
      require('./chunks/pad/chunk-014.wav') as number,
    ],
  },
  musicCont: {
    label: 'Music · track4',
    hint: 'ffmpeg -f segment -segment_time 1 on music-track4-15s.wav',
    assets: [
      require('./chunks/music/chunk-000.wav') as number,
      require('./chunks/music/chunk-001.wav') as number,
      require('./chunks/music/chunk-002.wav') as number,
      require('./chunks/music/chunk-003.wav') as number,
      require('./chunks/music/chunk-004.wav') as number,
      require('./chunks/music/chunk-005.wav') as number,
      require('./chunks/music/chunk-006.wav') as number,
      require('./chunks/music/chunk-007.wav') as number,
      require('./chunks/music/chunk-008.wav') as number,
      require('./chunks/music/chunk-009.wav') as number,
      require('./chunks/music/chunk-010.wav') as number,
      require('./chunks/music/chunk-011.wav') as number,
      require('./chunks/music/chunk-012.wav') as number,
      require('./chunks/music/chunk-013.wav') as number,
      require('./chunks/music/chunk-014.wav') as number,
    ],
  },
  tone: {
    label: 'Tone 440 Hz',
    hint: 'ffmpeg-segmented ~1s WAV chunks',
    assets: [
      require('./chunks/tone/chunk-000.wav') as number,
      require('./chunks/tone/chunk-001.wav') as number,
      require('./chunks/tone/chunk-002.wav') as number,
      require('./chunks/tone/chunk-003.wav') as number,
      require('./chunks/tone/chunk-004.wav') as number,
      require('./chunks/tone/chunk-005.wav') as number,
      require('./chunks/tone/chunk-006.wav') as number,
      require('./chunks/tone/chunk-007.wav') as number,
      require('./chunks/tone/chunk-008.wav') as number,
      require('./chunks/tone/chunk-009.wav') as number,
    ],
  },
};

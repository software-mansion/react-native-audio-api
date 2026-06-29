import { AudioContext } from 'react-native-audio-api';

type FileSourceNode = NonNullable<
  ReturnType<AudioContext['context']['createFileSource']>
>;

const AUDIO_URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/music/example-music-01.mp3';

const SUPPORTED_FORMATS = [
  'mp3',
  'wav',
  'aac',
  'flac',
  'ogg',
  'opus',
  'm4a',
  'mp4',
];

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

export const loadAudioTagSource = async (): Promise<ArrayBuffer> => {
  const response = await fetch(AUDIO_URL);
  return response.arrayBuffer();
};

const createFileSource = (
  ctx: AudioContext,
  options: {
    source: ArrayBuffer;
    loop?: boolean;
    volume?: number;
    playbackRate?: number;
    preservesPitch?: boolean;
  }
): FileSourceNode => {
  const node = ctx.context.createFileSource(options);
  if (!node) {
    throw new Error(
      'AudioTag test: createFileSource returned null (FFmpeg build required).'
    );
  }
  return node;
};

const teardown = (node: FileSourceNode) => {
  node.pause();
  node.disconnect();
};

export const audioTagBasicTest = async (
  ctx: AudioContext,
  source: ArrayBuffer,
  setInfo: (info: string) => void
) => {
  const node = createFileSource(ctx, { source, volume: 1 });

  node.start(ctx.currentTime);
  setInfo('AudioTag basic: playing 5s...');
  await sleep(5000);
  teardown(node);
  await sleep(300);

  setInfo(`AudioTag basic: done. duration=${node.duration.toFixed(2)}s`);
};

export const audioTagPauseResumeTest = async (
  ctx: AudioContext,
  source: ArrayBuffer,
  setInfo: (info: string) => void
) => {
  const node = createFileSource(ctx, { source, volume: 1 });

  node.start(ctx.currentTime);
  setInfo('AudioTag pause/resume: playing 3s...');
  await sleep(3000);

  node.pause();
  setInfo('AudioTag pause/resume: paused 1.5s...');
  await sleep(1500);

  node.start(ctx.currentTime);
  setInfo('AudioTag pause/resume: resumed 3s...');
  await sleep(3000);

  teardown(node);
  await sleep(300);
  setInfo('AudioTag pause/resume: done.');
};

export const audioTagSeekTest = async (
  ctx: AudioContext,
  source: ArrayBuffer,
  setInfo: (info: string) => void
) => {
  const node = createFileSource(ctx, { source, volume: 1 });

  node.start(ctx.currentTime);

  for (const seconds of [10, 0, 20, 5]) {
    setInfo(`AudioTag seek: seeking to ${seconds}s, playing 3s...`);
    node.seekToTime(seconds);
    await sleep(3000);
  }

  teardown(node);
  await sleep(300);
  setInfo('AudioTag seek: done.');
};

export const audioTagVolumeTest = async (
  ctx: AudioContext,
  source: ArrayBuffer,
  setInfo: (info: string) => void
) => {
  const node = createFileSource(ctx, { source, volume: 1 });

  node.start(ctx.currentTime);

  for (const volume of [1, 0.5, 0.1, 0.75, 0]) {
    setInfo(`AudioTag volume: ${volume} (2s)...`);
    node.volume = volume;
    await sleep(2000);
  }

  teardown(node);
  await sleep(300);
  setInfo('AudioTag volume: done.');
};

export const audioTagPlaybackRateTest = async (
  ctx: AudioContext,
  source: ArrayBuffer,
  setInfo: (info: string) => void
) => {
  for (const preservesPitch of [false, true]) {
    for (const rate of [0.5, 1, 1.5, 2]) {
      setInfo(
        `AudioTag rate: ${rate}x preservesPitch=${preservesPitch} (3s)...`
      );
      const node = createFileSource(ctx, {
        source,
        volume: 1,
        playbackRate: rate,
        preservesPitch,
      });
      node.start(ctx.currentTime);
      await sleep(3000);
      teardown(node);
      await sleep(300);
    }
  }

  setInfo('AudioTag rate: done.');
};

export const audioTagLoopTest = async (
  ctx: AudioContext,
  source: ArrayBuffer,
  setInfo: (info: string) => void
) => {
  const node = createFileSource(ctx, { source, volume: 1, loop: true });
  const playDuration = 12000;
  setInfo(`AudioTag loop: looping for ${playDuration / 1000}s...`);

  node.start(ctx.currentTime);
  await sleep(playDuration);

  teardown(node);
  await sleep(300);
  setInfo('AudioTag loop: done.');
};

export const audioTagFormatsTest = async (
  ctx: AudioContext,
  setInfo: (info: string) => void
) => {
  for (const format of SUPPORTED_FORMATS) {
    const url = `https://dl.espressif.com/dl/audio/gs-16b-2c-44100hz.${format}`;
    setInfo(`AudioTag formats: loading ${format}...`);

    try {
      const response = await fetch(url, {
        headers: {
          'User-Agent':
            'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
        },
      });
      const arrayBuffer = await response.arrayBuffer();

      const node = createFileSource(ctx, { source: arrayBuffer, volume: 1 });
      setInfo(
        `AudioTag formats: playing ${format} (duration=${node.duration.toFixed(2)}s)...`
      );
      node.start(ctx.currentTime);
      await sleep(4000);
      teardown(node);
      await sleep(300);
    } catch (error) {
      setInfo(`AudioTag formats: error playing ${format} - ${error}`);
      await sleep(1500);
    }
  }

  setInfo('AudioTag formats: done.');
};

import { AudioContext, AudioBuffer } from 'react-native-audio-api';

const AUDIO_URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/music/example-music-01.mp3';

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

export const loadAudioBuffer = async (
  ctx: AudioContext
): Promise<AudioBuffer> => {
  const response = await fetch(AUDIO_URL);
  const arrayBuffer = await response.arrayBuffer();
  return ctx.decodeAudioData(arrayBuffer);
};

export const audioBufferSourceBasicTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const source = ctx.createBufferSource();
  source.buffer = buffer;

  let endedFired = false;
  source.onEnded = () => {
    endedFired = true;
  };

  source.connect(ctx.destination);
  source.start(ctx.currentTime);
  setInfo('Basic: playing 5s...');
  await sleep(5000);
  source.stop();
  await sleep(300);

  setInfo(`Basic: done. onEnded fired: ${endedFired}`);
};

export const audioBufferSourceNaturalEndTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  // Use a short region so the source ends naturally without stop()
  const source = ctx.createBufferSource();
  source.buffer = buffer;

  let endedFired = false;
  source.onEnded = () => {
    endedFired = true;
  };

  const offset = 0;
  const duration = 3;
  source.connect(ctx.destination);
  source.start(ctx.currentTime, offset, duration);
  setInfo(`Natural end: playing ${duration}s region, waiting for onEnded...`);
  await sleep(duration * 1000 + 1000);

  setInfo(`Natural end: done. onEnded fired: ${endedFired}`);
};

export const audioBufferSourceOffsetDurationTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const cases: Array<{ offset: number; duration: number }> = [
    { offset: 0, duration: 3 },
    { offset: 5, duration: 3 },
    { offset: 10, duration: 4 },
    { offset: 20, duration: 3 },
  ];

  for (const { offset, duration } of cases) {
    setInfo(`Offset/duration: offset=${offset}s duration=${duration}s...`);
    const source = ctx.createBufferSource();
    source.buffer = buffer;
    source.connect(ctx.destination);
    source.start(ctx.currentTime, offset, duration);
    await sleep(duration * 1000 + 400);
  }

  setInfo('Offset/duration: done.');
};

export const audioBufferSourceScheduledStartTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const delay = 2;
  setInfo(`Scheduled start: will begin in ${delay}s...`);
  const source = ctx.createBufferSource();
  source.buffer = buffer;
  source.connect(ctx.destination);
  source.start(ctx.currentTime + delay, 0, 4);
  await sleep(delay * 1000 + 4000 + 400);
  setInfo('Scheduled start: done.');
};

export const audioBufferSourceLoopTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const loopStart = 2;
  const loopEnd = 5;
  const playDuration = 12000;
  setInfo(
    `Loop: loopStart=${loopStart}s loopEnd=${loopEnd}s, playing ${playDuration / 1000}s...`
  );

  const source = ctx.createBufferSource();
  source.buffer = buffer;
  source.loop = true;
  source.loopStart = loopStart;
  source.loopEnd = loopEnd;

  let loopCount = 0;
  source.onLoopEnded = () => {
    loopCount += 1;
  };

  source.connect(ctx.destination);
  source.start(ctx.currentTime);
  await sleep(playDuration);
  source.stop();
  await sleep(300);

  setInfo(`Loop: done. onLoopEnded fired ${loopCount} time(s).`);
};

export const audioBufferSourceLoopSkipTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const loopStart = 2;
  const loopEnd = 6;
  const playDuration = 10000;
  setInfo(
    `Loop skip: loopStart=${loopStart}s loopEnd=${loopEnd}s, playing ${playDuration / 1000}s...`
  );

  const source = ctx.createBufferSource();
  source.buffer = buffer;
  source.loop = true;
  source.loopSkip = true;
  source.loopStart = loopStart;
  source.loopEnd = loopEnd;

  source.connect(ctx.destination);
  source.start(ctx.currentTime);
  await sleep(playDuration);
  source.stop();
  await sleep(300);

  setInfo('Loop skip: done.');
};

export const audioBufferSourcePlaybackRateTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  for (const rate of [0.25, 0.5, 0.75, 1.0, 1.5, 2.0]) {
    setInfo(`Playback rate: ${rate}x (3s)...`);
    const source = ctx.createBufferSource();
    source.buffer = buffer;
    source.playbackRate.value = rate;
    source.connect(ctx.destination);
    source.start(ctx.currentTime);
    await sleep(3000);
    source.stop();
    await sleep(300);
  }

  setInfo('Playback rate: done.');
};

export const audioBufferSourceDetuneTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  for (const cents of [-2400, -1200, -600, 0, 600, 1200, 2400]) {
    setInfo(`Detune: ${cents} cents (2.5s)...`);
    const source = ctx.createBufferSource();
    source.buffer = buffer;
    source.detune.value = cents;
    source.connect(ctx.destination);
    source.start(ctx.currentTime);
    await sleep(2500);
    source.stop();
    await sleep(300);
  }

  setInfo('Detune: done.');
};

export const audioBufferSourceNegativePlaybackRateTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  for (const rate of [-0.5, -1.0, -2.0]) {
    setInfo(`Negative playback rate: ${rate}x (3s, reversed from end)...`);
    const source = ctx.createBufferSource();
    source.buffer = buffer;
    source.playbackRate.value = rate;
    source.connect(ctx.destination);
    // Start from the end of the buffer so there is content to read backwards
    source.start(ctx.currentTime, buffer.duration);
    await sleep(3000);
    source.stop();
    await sleep(300);
  }

  setInfo('Negative playback rate: done.');
};

export const audioBufferSourceNegativePlaybackRateLoopTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  // Reverse + loop: playback runs backward inside [loopStart, loopEnd] and wraps to
  // loopEnd again instead of stopping when it hits loopStart.
  const loopStart = 2;
  const loopEnd = 5;
  const playDuration = 10000;

  for (const rate of [-1.0, -0.75]) {
    setInfo(
      `Negative rate + loop: ${rate}x, loop [${loopStart}s, ${loopEnd}s], ${playDuration / 1000}s...`
    );
    const source = ctx.createBufferSource();
    source.buffer = buffer;
    source.playbackRate.value = rate;
    source.loop = true;
    source.loopStart = loopStart;
    source.loopEnd = loopEnd;
    source.connect(ctx.destination);
    // Start at the end of the loop region so the cursor begins inside [loopStart, loopEnd]
    source.start(ctx.currentTime, loopEnd);
    await sleep(playDuration);
    source.stop();
    await sleep(300);
  }

  setInfo('Negative rate + loop: done.');
};

export const audioBufferSourceLongPlaybackTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const playDuration = 60000;
  setInfo(`Long playback: looping for ${playDuration / 1000}s...`);

  const source = ctx.createBufferSource();
  source.buffer = buffer;
  source.loop = true;

  source.connect(ctx.destination);
  source.start(ctx.currentTime);
  await sleep(playDuration);
  source.stop();
  await sleep(300);

  setInfo('Long playback: done.');
};

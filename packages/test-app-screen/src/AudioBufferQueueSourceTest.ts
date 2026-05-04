import { AudioContext, AudioBuffer } from 'react-native-audio-api';

interface BufferEndedEvent {
  bufferId: string;
  isLastBufferInQueue: boolean;
}

const AUDIO_URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/music/example-music-02.mp3';

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

export const loadAudioBuffer = async (
  ctx: AudioContext
): Promise<AudioBuffer> => {
  const response = await fetch(AUDIO_URL);
  const arrayBuffer = await response.arrayBuffer();
  return ctx.decodeAudioData(arrayBuffer);
};

const sliceBuffer = (
  ctx: AudioContext,
  buffer: AudioBuffer,
  offsetSeconds: number,
  durationSeconds: number
): AudioBuffer => {
  const sampleRate = buffer.sampleRate;
  const startFrame = Math.max(0, Math.floor(offsetSeconds * sampleRate));
  const totalFrames = Math.floor(buffer.duration * sampleRate);
  const length = Math.min(
    Math.floor(durationSeconds * sampleRate),
    Math.max(0, totalFrames - startFrame)
  );
  const channels = buffer.numberOfChannels;
  const slice = ctx.createBuffer(channels, length, sampleRate);
  const tmp = new Float32Array(length);
  for (let ch = 0; ch < channels; ch++) {
    buffer.copyFromChannel(tmp, ch, startFrame);
    slice.copyToChannel(tmp, ch, 0);
  }
  return slice;
};

export const queueSourceBasicTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const slice = sliceBuffer(ctx, buffer, 0, 4);
  const source = ctx.createBufferQueueSource();

  let endedFired = 0;
  let lastEvent: BufferEndedEvent | null = null;
  source.onBufferEnded = (event: BufferEndedEvent) => {
    endedFired += 1;
    lastEvent = event;
  };

  const id = source.enqueueBuffer(slice);
  source.connect(ctx.destination);
  source.start(ctx.currentTime, 0);
  setInfo(`Basic: playing single 4s buffer (id=${id})...`);
  await sleep(4500);
  source.stop();
  await sleep(300);

  const lastIsLast: string = lastEvent
    ? String((lastEvent as BufferEndedEvent).isLastBufferInQueue)
    : 'n/a';
  setInfo(
    `Basic: done. onBufferEnded fired ${endedFired} time(s), last isLast=${lastIsLast}.`
  );
};

export const queueSourceMultipleBuffersTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const slices = [
    sliceBuffer(ctx, buffer, 0, 3),
    sliceBuffer(ctx, buffer, 3, 3),
    sliceBuffer(ctx, buffer, 6, 3),
  ];
  const source = ctx.createBufferQueueSource();

  const endedEvents: BufferEndedEvent[] = [];
  source.onBufferEnded = (event: BufferEndedEvent) => {
    endedEvents.push(event);
  };

  const ids = slices.map((s) => source.enqueueBuffer(s));
  source.connect(ctx.destination);
  source.start(ctx.currentTime, 0);
  setInfo(
    `Multiple: enqueued 3 slices [${ids.join(', ')}], expecting seamless playback...`
  );
  await sleep(3 * 3 * 1000 + 500);
  source.stop();
  await sleep(300);

  const lastFlag = endedEvents.length
    ? endedEvents[endedEvents.length - 1].isLastBufferInQueue
    : false;
  setInfo(
    `Multiple: done. onBufferEnded fired ${endedEvents.length} time(s); last isLastBufferInQueue=${lastFlag}.`
  );
};

export const queueSourceEnqueueWhilePlayingTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const source = ctx.createBufferQueueSource();
  let endedCount = 0;
  source.onBufferEnded = () => {
    endedCount += 1;
  };

  source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 3));
  source.connect(ctx.destination);
  source.start(ctx.currentTime, 0);
  setInfo('Enqueue-while-playing: started with 1 buffer (3s)...');

  await sleep(1500);
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 5, 3));
  setInfo('Enqueue-while-playing: appended 2nd buffer at 1.5s...');

  await sleep(2500);
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 10, 3));
  setInfo('Enqueue-while-playing: appended 3rd buffer at 4s...');

  await sleep(4000);
  source.stop();
  await sleep(300);

  setInfo(
    `Enqueue-while-playing: done. onBufferEnded fired ${endedCount} time(s).`
  );
};

export const queueSourceDequeueTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const source = ctx.createBufferQueueSource();
  const playedIds: string[] = [];
  source.onBufferEnded = (event: BufferEndedEvent) => {
    playedIds.push(event.bufferId);
  };

  const id1 = source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 3));
  const id2 = source.enqueueBuffer(sliceBuffer(ctx, buffer, 6, 3));
  const id3 = source.enqueueBuffer(sliceBuffer(ctx, buffer, 12, 3));

  source.connect(ctx.destination);
  source.start(ctx.currentTime, 0);
  setInfo(
    `Dequeue: enqueued [${id1}, ${id2}, ${id3}], will dequeue middle (${id2}) at 1s...`
  );

  await sleep(1000);
  source.dequeueBuffer(id2);
  setInfo(`Dequeue: removed buffer ${id2}, expect ${id1} → ${id3} only.`);

  await sleep(7000);
  source.stop();
  await sleep(300);

  setInfo(
    `Dequeue: done. onBufferEnded ids: [${playedIds.join(', ')}] (should not contain ${id2}).`
  );
};

export const queueSourceClearBuffersTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const source = ctx.createBufferQueueSource();
  let endedCount = 0;
  source.onBufferEnded = () => {
    endedCount += 1;
  };

  source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 4));
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 6, 4));
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 12, 4));

  source.connect(ctx.destination);
  source.start(ctx.currentTime, 0);
  setInfo('Clear: enqueued 3 buffers, will clearBuffers() at 2s...');

  await sleep(2000);
  source.clearBuffers();
  setInfo('Clear: cleared queue — expect silence for 2s, then enqueue more.');

  await sleep(2000);
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 18, 3));
  setInfo('Clear: enqueued a new buffer after clear, expect playback resumes.');

  await sleep(3500);
  source.stop();
  await sleep(300);

  setInfo(`Clear: done. onBufferEnded fired ${endedCount} time(s).`);
};

export const queueSourcePauseResumeTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const source = ctx.createBufferQueueSource();
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 4));
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 6, 4));
  source.connect(ctx.destination);
  source.start(ctx.currentTime, 0);
  setInfo('Pause/resume: playing... will pause at 2s.');

  await sleep(2000);
  source.pause();
  setInfo('Pause/resume: paused for 1.5s.');

  await sleep(1500);
  source.start(ctx.currentTime, 0);
  setInfo('Pause/resume: resumed via start().');

  await sleep(6500);
  source.stop();
  await sleep(300);

  setInfo('Pause/resume: done.');
};

export const queueSourceScheduledStartTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const delay = 2;
  const source = ctx.createBufferQueueSource();
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 4));
  source.connect(ctx.destination);
  source.start(ctx.currentTime + delay, 0);
  setInfo(`Scheduled start: silence for ${delay}s, then 4s of audio...`);
  await sleep(delay * 1000 + 4000 + 400);
  setInfo('Scheduled start: done.');
};

export const queueSourceStartOffsetTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const offset = 2;
  const source = ctx.createBufferQueueSource();
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 6));
  source.connect(ctx.destination);
  source.start(ctx.currentTime, offset);
  setInfo(
    `Start offset: 6s buffer played from ${offset}s offset (expect ~4s)...`
  );
  await sleep(6000);
  source.stop();
  await sleep(300);
  setInfo('Start offset: done.');
};

export const queueSourcePlaybackRateTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  for (const rate of [0.5, 1.0, 1.5, 2.0]) {
    setInfo(`Playback rate: ${rate}x (3s)...`);
    const source = ctx.createBufferQueueSource();
    source.playbackRate.value = rate;
    source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 3));
    source.connect(ctx.destination);
    source.start(ctx.currentTime, 0);
    await sleep(3000);
    source.stop();
    await sleep(300);
  }
  setInfo('Playback rate: done.');
};

export const queueSourceDetuneTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  for (const cents of [-1200, 0, 1200]) {
    setInfo(`Detune: ${cents} cents (2.5s)...`);
    const source = ctx.createBufferQueueSource();
    source.detune.value = cents;
    source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 3));
    source.connect(ctx.destination);
    source.start(ctx.currentTime, 0);
    await sleep(2500);
    source.stop();
    await sleep(300);
  }
  setInfo('Detune: done.');
};

export const queueSourceLastFlagTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const source = ctx.createBufferQueueSource();
  const events: BufferEndedEvent[] = [];
  source.onBufferEnded = (event: BufferEndedEvent) => {
    events.push(event);
  };

  source.enqueueBuffer(sliceBuffer(ctx, buffer, 0, 2));
  source.enqueueBuffer(sliceBuffer(ctx, buffer, 4, 2));

  source.connect(ctx.destination);
  source.start(ctx.currentTime, 0);
  setInfo('isLastBufferInQueue flag: playing 2 short buffers...');

  await sleep(5000);
  source.stop();
  await sleep(300);

  const summary = events
    .map((e) => `${e.bufferId}:${e.isLastBufferInQueue ? 'last' : 'not-last'}`)
    .join(', ');
  setInfo(`isLastBufferInQueue flag: events [${summary}].`);
};

export const queueSourceLongPlaybackTest = async (
  ctx: AudioContext,
  buffer: AudioBuffer,
  setInfo: (info: string) => void
) => {
  const totalDurationMs = 60000;
  const sliceLen = 4;
  const source = ctx.createBufferQueueSource();
  let endedCount = 0;
  source.onBufferEnded = () => {
    endedCount += 1;
  };
  source.connect(ctx.destination);

  const bufferDuration = buffer.duration;
  let nextOffset = 0;
  const enqueueNext = () => {
    if (nextOffset + sliceLen > bufferDuration) {
      nextOffset = 0;
    }
    source.enqueueBuffer(sliceBuffer(ctx, buffer, nextOffset, sliceLen));
    nextOffset += sliceLen;
  };
  for (let i = 0; i < 3; i++) {
    enqueueNext();
  }

  source.start(ctx.currentTime, 0);
  setInfo(
    `Long playback: streaming ${sliceLen}s slices for ${totalDurationMs / 1000}s...`
  );

  const start = Date.now();
  while (Date.now() - start < totalDurationMs) {
    await sleep(sliceLen * 1000);
    enqueueNext();
    setInfo(
      `Long playback: ${Math.round((Date.now() - start) / 1000)}s elapsed, onBufferEnded count=${endedCount}.`
    );
  }

  source.stop();
  await sleep(500);
  setInfo(`Long playback: done. onBufferEnded fired ${endedCount} time(s).`);
};

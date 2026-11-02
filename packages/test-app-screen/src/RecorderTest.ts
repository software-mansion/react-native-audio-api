import { Platform } from 'react-native';
import {
  AudioContext,
  AudioRecorder,
  AudioBuffer,
  FileFormat,
} from 'react-native-audio-api';
import { isTestRunInterrupted, sleep } from './interruption';

export const recorderTest = (
  audioContextRef: React.RefObject<AudioContext | null>,
  buffers: AudioBuffer[]
) => {
  const recorder = new AudioRecorder();

  recorder.onAudioReady(
    {
      sampleRate: audioContextRef.current!.sampleRate,
      bufferLength: audioContextRef.current!.sampleRate * 0.1,
      channelCount: 1,
    },
    (event) => {
      const { buffer, numFrames } = event;
      console.log('Audio recorder buffer ready:', numFrames);
      buffers.push(buffer);
    }
  );
  void recorder.start();
  return recorder;
};

export const recorderPlaybackTest = (
  audioContextRef: React.RefObject<AudioContext | null>,
  buffers: AudioBuffer[]
) => {
  let nextStartAt = audioContextRef.current!.currentTime + 0.1;
  for (let i = 0; i < buffers.length; i++) {
    const source = audioContextRef.current!.createBufferSource();
    source.buffer = buffers[i];
    source.connect(audioContextRef.current!.destination);
    source.start(nextStartAt);
    nextStartAt += buffers[i].duration;
  }
};

// Formats the current platform can encode, per the audio recorder docs matrix.
const ENCODABLE_FORMATS: FileFormat[] =
  Platform.OS === 'ios'
    ? [
        FileFormat.Wav,
        FileFormat.M4A,
        FileFormat.Flac,
        FileFormat.OpusOgg,
        FileFormat.Adts,
        FileFormat.Caf,
        FileFormat.Aiff,
        FileFormat.Alac,
        FileFormat.Ulaw,
        FileFormat.Alaw,
      ]
    : [
        FileFormat.Wav,
        FileFormat.M4A,
        FileFormat.Flac,
        FileFormat.OpusOgg,
        FileFormat.Adts,
        FileFormat.OpusWebm,
        FileFormat.VorbisWebm,
      ];

const RECORD_DURATION_MS = 2000;
const PLAYBACK_DURATION_MS = 1500;

const recordToFile = async (format: FileFormat): Promise<string> => {
  const recorder = new AudioRecorder();
  recorder.enableFileOutput({
    format,
    channelCount: 1,
    fileName: `format_test_${FileFormat[format]}`,
  });

  const started = await recorder.start();
  if (started.status === 'error') {
    throw new Error(`start failed: ${started.message}`);
  }
  try {
    await sleep(RECORD_DURATION_MS);
  } catch (error) {
    await recorder.stop();
    throw error;
  }

  const stopped = await recorder.stop();
  if (stopped.status === 'error') {
    throw new Error(`stop failed: ${stopped.message}`);
  }
  if (stopped.paths.length === 0) {
    throw new Error('stop returned no file');
  }
  return stopped.paths[0];
};

const toFileUri = (path: string) =>
  path.startsWith('file://') ? path : `file://${path}`;

export const recorderFormatsTest = async (
  ctx: AudioContext,
  setInfo: (info: string) => void
) => {
  const results: string[] = [];

  for (const format of ENCODABLE_FORMATS) {
    const name = FileFormat[format];
    setInfo(`Recorder formats: recording ${name}...`);

    let path: string;
    try {
      path = await recordToFile(format);
    } catch (error) {
      if (isTestRunInterrupted()) {
        throw error;
      }
      results.push(`${name}: encode ❌ (${String(error)})`);
      continue;
    }

    try {
      const buffer = await ctx.decodeAudioData(toFileUri(path));
      setInfo(
        `Recorder formats: playing ${name} (${buffer.duration.toFixed(2)}s)...`
      );
      const source = ctx.createBufferSource();
      source.buffer = buffer;
      source.connect(ctx.destination);
      source.start();
      await sleep(PLAYBACK_DURATION_MS);
      source.stop();
      results.push(
        `${name}: encode ✅ decode ✅ (${buffer.duration.toFixed(2)}s)`
      );
    } catch (error) {
      if (isTestRunInterrupted()) {
        throw error;
      }
      results.push(`${name}: encode ✅ decode ❌ (${String(error)})`);
    }
  }

  setInfo(`Recorder formats (${Platform.OS}):\n${results.join('\n')}`);
};

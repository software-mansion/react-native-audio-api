import {
  AudioBuffer,
  AudioContext,
  AudioManager,
  AudioRecorder,
  FileDirectory,
  FileFormat,
} from 'react-native-audio-api';

import { BUFFER_LENGTH, MIN_DECODED_DURATION_SECONDS } from './constants';
import StressPlaybackController from './StressPlaybackController';
import type { RecordingCapture, ReadyResources } from './types';

interface RecorderHostShape {
  isPaused?: unknown;
}

export default class StressResourceOwner {
  public context: AudioContext | null = null;
  public recorder: AudioRecorder | null = null;
  public playback: StressPlaybackController | null = null;
  public assetBuffer: AudioBuffer | null = null;
  public lastRecorderError: string | null = null;
  public callbackCount = 0;
  public lastCallbackFrames = 0;

  private readonly asset: string | number;

  constructor(asset: string | number) {
    this.asset = asset;
  }

  async recreate(): Promise<void> {
    await this.cleanup();

    const context = new AudioContext();
    const recorder = new AudioRecorder();
    const recorderHost = (
      recorder as unknown as { recorder?: RecorderHostShape }
    ).recorder;

    if (typeof recorderHost?.isPaused !== 'function') {
      throw new Error(
        `Recorder host object is missing isPaused(); typeof isPaused=${typeof recorderHost?.isPaused}`
      );
    }

    const fileOutputResult = recorder.enableFileOutput({
      channelCount: 1,
      directory: FileDirectory.Cache,
      fileNamePrefix: 'audio-pipeline-stress',
      format: FileFormat.M4A,
      subDirectory: 'AudioPipelineStress',
    });

    if (fileOutputResult.status === 'error') {
      throw new Error(
        `Failed to enable recorder file output: ${fileOutputResult.message}`
      );
    }

    recorder.onError((event) => {
      this.lastRecorderError = event.message;
    });

    this.context = context;
    this.recorder = recorder;
    this.playback = new StressPlaybackController(context);
    this.assetBuffer = await context.decodeAudioData(this.asset);
    this.lastRecorderError = null;
    this.callbackCount = 0;
    this.lastCallbackFrames = 0;
  }

  getReadyResources(): ReadyResources {
    if (
      !this.context ||
      !this.recorder ||
      !this.playback ||
      !this.assetBuffer
    ) {
      throw new Error('Audio pipeline resources are not ready');
    }

    return {
      assetBuffer: this.assetBuffer,
      context: this.context,
      playback: this.playback,
      recorder: this.recorder,
    };
  }

  configureRecorderTap(): void {
    const { context, recorder } = this.getReadyResources();
    this.callbackCount = 0;
    this.lastCallbackFrames = 0;
    this.lastRecorderError = null;

    const callbackResult = recorder.onAudioReady(
      {
        sampleRate: context.sampleRate,
        channelCount: 1,
        bufferLength: BUFFER_LENGTH,
      },
      (event) => {
        this.callbackCount += 1;
        this.lastCallbackFrames = event.numFrames;
      }
    );

    if (callbackResult.status === 'error') {
      throw new Error(
        `Failed to attach recorder callback: ${callbackResult.message}`
      );
    }
  }

  startRecording(fileNameOverride: string): void {
    const { recorder } = this.getReadyResources();
    const result = recorder.start({ fileNameOverride });

    if (result.status === 'error') {
      throw new Error(`Failed to start recording: ${result.message}`);
    }
  }

  tryStartRecording(fileNameOverride: string) {
    const { recorder } = this.getReadyResources();
    return recorder.start({ fileNameOverride });
  }

  async stopRecordingAndDecode(): Promise<RecordingCapture> {
    const { context, recorder } = this.getReadyResources();

    const stopResult = recorder.stop();
    recorder.clearOnAudioReady();

    if (stopResult.status === 'error') {
      throw new Error(`Failed to stop recording: ${stopResult.message}`);
    }

    if (!stopResult.paths.length) {
      throw new Error('Recorder stop returned an empty file path');
    }

    const filePath = stopResult.paths[0];

    const decodedBuffer = await context.decodeAudioData(filePath);

    if (decodedBuffer.duration < MIN_DECODED_DURATION_SECONDS) {
      throw new Error(
        `Decoded buffer is too short: ${decodedBuffer.duration.toFixed(3)}s`
      );
    }

    return {
      decodedBuffer,
      fileDurationSeconds: stopResult.duration,
      path: filePath,
    };
  }

  async cleanup(): Promise<void> {
    const recorder = this.recorder;
    const playback = this.playback;
    const context = this.context;

    try {
      playback?.stop();
    } catch {}

    try {
      recorder?.clearOnAudioReady();
    } catch {}

    try {
      recorder?.clearOnError();
    } catch {}

    try {
      if (recorder && (recorder.isRecording() || recorder.isPaused())) {
        recorder.stop();
      }
    } catch {}

    try {
      recorder?.disableFileOutput();
    } catch {}

    try {
      if (context?.state === 'running') {
        await context.suspend();
      }
    } catch {}

    try {
      await context?.close();
    } catch {}

    try {
      await AudioManager.setAudioSessionActivity(false);
    } catch {}

    this.context = null;
    this.recorder = null;
    this.playback = null;
    this.assetBuffer = null;
    this.lastRecorderError = null;
    this.callbackCount = 0;
    this.lastCallbackFrames = 0;
  }
}

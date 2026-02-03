import { AudioApiError } from '../errors';
import { AudioEventEmitter, AudioEventSubscription } from '../events';
import {
  OnAudioReadyEventType,
  OnRecorderErrorEventType,
} from '../events/types';
import { IAudioRecorder } from '../interfaces';
import {
  AudioRecorderCallbackOptions,
  AudioRecorderFileOptions,
  AudioRecorderStartOptions,
  FileDirectory,
  FileFormat,
  FileInfo,
  Result,
} from '../types';
import FilePreset from '../utils/filePresets';
import AudioBuffer from './AudioBuffer';
import RecorderAdapterNode from './RecorderAdapterNode';

// Enforces default options, making sure that all properties are defined
// for the contract with native code.
function withDefaultOptions(
  inOptions: AudioRecorderFileOptions
): Required<AudioRecorderFileOptions> {
  return {
    directory: FileDirectory.Cache,
    subDirectory: 'AudioAPI',
    fileNamePrefix: 'recording',
    channelCount: 2,
    format: FileFormat.M4A,
    batchDurationSeconds: 0,
    preset: FilePreset.High,
    androidFlushIntervalMs: 500,
    ...inOptions,
  };
}

export default class AudioRecorder {
  protected onAudioReadySubscription: AudioEventSubscription | null = null;
  protected onErrorSubscription: AudioEventSubscription | null = null;
  protected readonly recorder: IAudioRecorder;
  protected options_: AudioRecorderFileOptions | null = null;
  private isFileOutputEnabled: boolean = false;

  protected readonly audioEventEmitter = new AudioEventEmitter(
    global.AudioEventEmitter
  );

  constructor() {
    this.recorder = global.createAudioRecorder();
  }

  enableFileOutput(
    // TODO: Re-enable 'batchDurationSeconds' once supported
    options?: Omit<AudioRecorderFileOptions, 'batchDurationSeconds'>
  ): Result<{ path: string }> {
    this.options_ = options || {};
    const parsedOptions = withDefaultOptions(this.options_);
    const result = this.recorder.enableFileOutput(parsedOptions);
    this.isFileOutputEnabled = true;

    return result;
  }

  public get options(): AudioRecorderFileOptions | null {
    return this.options_;
  }

  disableFileOutput(): void {
    this.options_ = null;
    this.recorder.disableFileOutput();
    this.isFileOutputEnabled = false;
  }

  /** Starts the audio recording process with configured output options */
  start(options?: AudioRecorderStartOptions): Result<{ path: string }> {
    if (!this.isFileOutputEnabled) {
      this.recorder.start();
      return { status: 'success', path: '' };
    }

    return this.recorder.start(options?.fileNameOverride);
  }

  /** Stops the audio recording process and releases internal resources */
  stop(): Result<FileInfo> {
    return this.recorder.stop();
  }

  /** Pauses the audio recording process without tearing down anything */
  pause(): void {
    this.recorder.pause();
  }

  /** Resumes the audio recording process after being paused */
  resume(): void {
    this.recorder.resume();
  }

  /**
   * Connects a {@link RecorderAdapterNode} to the recorder’s audio graph.
   *
   * Each node can only be connected once. Attempting to connect a node multiple
   * times will throw an error.
   *
   * @param node - The adapter node to connect to the recorder.
   * @throws If the node has already been connected.
   */
  connect(node: RecorderAdapterNode): void {
    if (node.wasConnected) {
      throw new AudioApiError(
        'RecorderAdapterNode cannot be connected more than once. Refer to the documentation for more details.'
      );
    }

    node.wasConnected = true;
    this.recorder.connect(node.getNode());
  }

  /**
   * Disconnects the recorder from all connected adapter nodes.
   *
   * After calling this method, any connected {@link RecorderAdapterNode} will no
   * longer receive audio data until reconnected.
   */
  disconnect(): void {
    this.recorder.disconnect();
  }

  /**
   * Registers a callback to receive raw audio data during an active recording
   * session.
   *
   * The callback is periodically invoked with audio buffers that match the
   * preferred configuration provided in `options`. These parameters (sample
   * rate, buffer length, and channel count) guide how audio data is chunked and
   * delivered, though the exact values may vary depending on device
   * capabilities. Values may vary depending on device capabilities.
   *
   * @param options - Preferred configuration for the audio buffers delivered to
   *   the callback.
   * @param callback - Function invoked each time a new audio buffer is
   *   available. The callback receives an {@link OnAudioReadyEventType} object
   *   containing the audio data and associated metadata.
   */
  onAudioReady(
    options: AudioRecorderCallbackOptions,
    callback: (event: OnAudioReadyEventType) => void
  ): Result<void> {
    if (this.onAudioReadySubscription) {
      this.recorder.clearOnAudioReady();
      this.onAudioReadySubscription.remove();
      this.onAudioReadySubscription = null;
    }

    this.onAudioReadySubscription =
      this.audioEventEmitter.addAudioEventListener('audioReady', (event) => {
        const audioBuffer = new AudioBuffer(event.buffer);
        callback({
          ...event,
          buffer: audioBuffer,
        });
      });

    return this.recorder.setOnAudioReady({
      sampleRate: options.sampleRate,
      bufferLength: options.bufferLength,
      channelCount: options.channelCount,
      callbackId: this.onAudioReadySubscription.subscriptionId,
    });
  }

  /**
   * Removes the previously registered audio data callback, if any.
   *
   * This stops further `onAudioReady` events from being delivered during
   * recording. Calling this method is safe even if no callback is currently
   * registered.
   */
  clearOnAudioReady(): void {
    if (!this.onAudioReadySubscription) {
      return;
    }

    this.recorder.clearOnAudioReady();

    this.onAudioReadySubscription.remove();
    this.onAudioReadySubscription = null;
  }

  isRecording(): boolean {
    return this.recorder.isRecording();
  }

  isPaused(): boolean {
    return this.recorder.isPaused();
  }

  getCurrentDuration(): number {
    return this.recorder.getCurrentDuration();
  }

  onError(callback: (error: OnRecorderErrorEventType) => void): void {
    if (this.onErrorSubscription) {
      this.recorder.clearOnError();
      this.onErrorSubscription.remove();
      this.onErrorSubscription = null;
    }

    this.onErrorSubscription = this.audioEventEmitter.addAudioEventListener(
      'recorderError',
      callback
    );

    this.recorder.setOnError({
      callbackId: this.onErrorSubscription.subscriptionId,
    });
  }

  clearOnError(): void {
    if (!this.onErrorSubscription) {
      return;
    }

    this.recorder.clearOnError();

    this.onErrorSubscription.remove();
    this.onErrorSubscription = null;
  }
}

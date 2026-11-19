import { AudioEventEmitter, AudioEventSubscription } from '../events';
import { NotSupportedError } from '../errors';
import {
  OnAudioReadyEventType,
  OnRecorderErrorEventType,
} from '../events/types';
import { IAudioRecorder, IRecorderAdapterNode } from '../jsi-interfaces';
import {
  AudioRecorderCallbackOptions,
  AudioRecorderFileOptions,
  AudioRecorderOptions,
  FileDirectory,
  FileFormat,
  FileInfo,
  RecordingClock,
  Result,
} from '../types';
// Imported for {@link concatAudioFiles} in enableFileOutput docs.
// eslint-disable-next-line @typescript-eslint/no-unused-vars -- JSDoc link target
import { concatAudioFiles } from './AudioFileUtils';
import FilePreset from '../utils/filePresets';
import AudioBuffer from './AudioBuffer';
import AudioConsumer from './AudioConsumer';
import type AudioNode from './AudioNode';
import type BaseAudioContext from './BaseAudioContext';

// Enforces default file output options, making sure that all properties are
// defined for the contract with native code.
function withDefaultFileOptions(
  inOptions: AudioRecorderFileOptions
): Required<AudioRecorderFileOptions> {
  return {
    directory: FileDirectory.Cache,
    subDirectory: 'AudioAPI',
    fileName: '',
    channelCount: 2,
    format: FileFormat.M4A,
    preset: FilePreset.High,
    androidFlushIntervalMs: 500,
    rotateIntervalBytes: 0,
    ...inOptions,
  };
}

export default class AudioRecorder {
  protected onErrorSubscription: AudioEventSubscription | null = null;
  protected readonly recorder: IAudioRecorder;
  protected options_: AudioRecorderFileOptions | null = null;
  /**
   * While `onAudioReady` is set: the consumer added for it and the event
   * subscription it delivers to.
   */
  private audioReady: {
    consumer: AudioConsumer;
    subscription: AudioEventSubscription;
  } | null = null;

  /**
   * While connected: the adapter node, kept alive here, and the consumer added
   * for it.
   */
  private adapter: {
    node: IRecorderAdapterNode;
    consumer: AudioConsumer;
  } | null = null;

  /**
   * Kept while added, so no added consumer is garbage-collected under the
   * recorder.
   */
  private readonly consumers = new Set<AudioConsumer>();

  protected readonly audioEventEmitter = new AudioEventEmitter(
    globalThis.AudioEventEmitter
  );

  private readonly durationClock: RecordingClock;

  constructor(options?: AudioRecorderOptions) {
    this.recorder = globalThis.createAudioRecorder(options ?? {});
    this.durationClock = this.recorder.clock;
  }

  static isRecordingOngoing(): boolean {
    return globalThis.isRecordingOngoing?.() ?? false;
  }

  static consumeLastRecordingResult(): FileInfo | null {
    return globalThis.consumeLastRecordingResult?.() ?? null;
  }

  /**
   * Enables writing recorded audio to a file using the provided options.
   *
   * When {@link AudioRecorderFileOptions.rotateIntervalBytes} is greater than
   * `0`, the recorder splits output into multiple segment files. After
   * recording, join them with {@link concatAudioFiles} from the package entry
   * point.
   *
   * **Rotation size:** very low `rotateIntervalBytes` values rotate often and
   * cause audible artifacts at segment boundaries after concatenation —
   * especially for encoded formats such as M4A, where each segment starts a
   * fresh AAC encode. Practical starting points: about **512 KB–1 MB** for WAV
   * and **≥ 200 KB** for M4A. This option controls segment file size, not
   * recording memory use.
   *
   * The file sample rate (`preset.sampleRate`) must be at most 48000 Hz: the
   * iOS and Android system encoders accept no higher rate.
   *
   * @throws {@link NotSupportedError} When the format cannot be encoded by this
   *   platform's system encoders, the file options are invalid (including a
   *   sample rate above 48000 Hz), or a recording session is active. The
   *   previous file-output configuration stays in effect.
   */
  enableFileOutput(options?: AudioRecorderFileOptions): void {
    const requestedOptions = options || {};
    const result = this.recorder.enableFileOutput(
      withDefaultFileOptions(requestedOptions)
    );

    if (result.status === 'success') {
      this.options_ = requestedOptions;
    } else {
      throw new NotSupportedError(
        'AudioRecorder.enableFileOutput failed: ' + result.message
      );
    }
  }

  public get options(): AudioRecorderFileOptions | null {
    return this.options_;
  }

  /**
   * Disables file output. When called mid-recording, the current file is
   * finalized first; the returned promise resolves once it is closed.
   */
  disableFileOutput(): Promise<void> {
    this.options_ = null;
    return this.recorder.disableFileOutput();
  }

  /**
   * Starts the audio recording process with configured output options.
   *
   * Rejects when the platform refuses to start the recording.
   */
  start(): Promise<void> {
    return this.recorder.start();
  }

  /**
   * Stops the audio recording process and releases internal resources.
   *
   * Resolves with the info of the finalized output files. Rejects when the
   * recorder was not started, or the output files could not be finalized.
   */
  stop(): Promise<FileInfo> {
    return this.recorder.stop();
  }

  /** Pauses the audio recording process without tearing down anything */
  pause(): Promise<void> {
    return this.recorder.pause();
  }

  /**
   * Resumes the audio recording process after being paused. Does nothing when
   * the recording is already running. Rejects when the recorder was not
   * started, or the platform refuses to resume, in which case the recording
   * stays paused.
   */
  resume(): Promise<void> {
    return this.recorder.resume();
  }

  /** Connects the recorder to a destination node. */
  connect(context: BaseAudioContext, destination: AudioNode): AudioNode {
    if (this.adapter == null) {
      const node = context.context.createRecorderAdapter();
      this.adapter = { node, consumer: new AudioConsumer(node.consumer) };
      this.addConsumer(this.adapter.consumer);
    }

    // @ts-expect-error node is protected, but we need to access it here
    this.adapter.node.connect(destination.node);

    return destination;
  }

  /**
   * Disconnects the recorder from the audio graph. The destination passed to
   * {@link connect} no longer receives recorded audio until connected again.
   */
  disconnect(): void {
    if (this.adapter != null) {
      this.removeConsumer(this.adapter.consumer);
      this.adapter = null;
    }
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
    this.clearOnAudioReady();

    const subscription = this.audioEventEmitter.addAudioEventListener(
      'audioReady',
      (event) => {
        const audioBuffer = new AudioBuffer(event.buffer);
        callback({
          ...event,
          buffer: audioBuffer,
        });
      }
    );
    const consumer = new AudioConsumer(
      globalThis.createAudioReadyConsumer({
        sampleRate: options.sampleRate,
        bufferLength: options.bufferLength,
        channelCount: options.channelCount,
        callbackId: subscription.subscriptionId,
      })
    );

    const result = this.addConsumer(consumer);
    if (result.status === 'success') {
      this.audioReady = { consumer, subscription };
    } else {
      subscription.remove();
    }
    return result;
  }

  /**
   * Removes the previously registered audio data callback, if any.
   *
   * This stops further `onAudioReady` events from being delivered during
   * recording. Calling this method is safe even if no callback is currently
   * registered.
   */
  clearOnAudioReady(): void {
    if (this.audioReady == null) {
      return;
    }

    this.removeConsumer(this.audioReady.consumer);
    this.audioReady.subscription.remove();
    this.audioReady = null;
  }

  /**
   * Feeds every recorded buffer to `consumer` as well, from the next session
   * on, or at once when a recording is running. A consumer can be added once;
   * adding it again fails.
   *
   * @returns An error result when the consumer cannot be prepared for the
   *   running recording or is added already; it is then not added.
   */
  addConsumer(consumer: AudioConsumer): Result<void> {
    const result = this.recorder.addConsumer(consumer.consumer);
    if (result.status === 'success') {
      this.consumers.add(consumer);
    }
    return result;
  }

  /**
   * Stops feeding `consumer`, closing it when a recording is running. Does
   * nothing when it was not added.
   */
  removeConsumer(consumer: AudioConsumer): void {
    this.recorder.removeConsumer(consumer.consumer);
    this.consumers.delete(consumer);
  }

  isRecording(): boolean {
    return this.recorder.isRecording();
  }

  isPaused(): boolean {
    return this.recorder.isPaused();
  }

  getCurrentDuration(): number {
    return this.durationClock.currentDuration;
  }

  /**
   * The duration as a {@link RecordingClock}: capture it in a UI worklet to read
   * the duration on the UI thread, every frame if needed.
   */
  get clock(): RecordingClock {
    return this.durationClock;
  }

  get inputLatency(): number {
    return this.recorder.inputLatency;
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

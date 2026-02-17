import {
  AudioContextOptions,
  AudioRecorderCallbackOptions,
  AudioRecorderFileOptions,
  AudioRecorderStartOptions,
  AudioWorkletRuntime,
  BiquadFilterType,
  ChannelCountMode,
  ChannelInterpretation,
  ContextState,
  FileDirectory,
  FileFormat,
  FileInfo,
  FilePresetType,
  OfflineAudioContextOptions,
  OscillatorType,
  OverSampleType,
  Result,
  AnalyserOptions,
  AudioBufferSourceOptions,
  BaseAudioBufferSourceOptions,
  BiquadFilterOptions,
  ConstantSourceOptions,
  ConvolverOptions,
  DelayOptions,
  GainOptions,
  OscillatorOptions,
  PeriodicWaveOptions,
  StereoPannerOptions,
  StreamerOptions,
  WaveShaperOptions,
} from '../types';

/* eslint-disable no-useless-constructor */

const noop = () => {};

class MockEventSubscription {
  public subscriptionId: string = Number(1).toString();

  remove = noop;
}

class MockAudioEventEmitter {
  private listeners: {
    [event: string]: Array<
      (event: Partial<Record<string, unknown>> | undefined) => void
    >;
  } = {};

  addAudioEventListener(
    event: string,
    callback: (event: Partial<Record<string, unknown>> | undefined) => void
  ): MockEventSubscription {
    if (!this.listeners[event]) {
      this.listeners[event] = [];
    }
    this.listeners[event].push(callback);
    const subscription = new MockEventSubscription();

    subscription.remove = () => {
      const index = this.listeners[event]?.indexOf(callback);
      if (index && index > -1) {
        this.listeners[event].splice(index, 1);
      }
    };

    return subscription;
  }

  emit(event: string, data: Record<string, unknown>): void {
    this.listeners[event]?.forEach((callback) => callback(data));
  }
}

class AudioParamMock {
  private _value: number = 0;
  public defaultValue: number = 0;
  public minValue: number = -3.4028235e38;
  public maxValue: number = 3.4028235e38;

  constructor(_audioParam: unknown, _context: BaseAudioContextMock) {}

  get value(): number {
    return this._value;
  }

  set value(newValue: number) {
    this._value = newValue;
  }

  public setValueAtTime(value: number, _startTime: number): AudioParamMock {
    this._value = value;
    return this;
  }

  public linearRampToValueAtTime(
    value: number,
    _endTime: number
  ): AudioParamMock {
    this._value = value;
    return this;
  }

  public exponentialRampToValueAtTime(
    value: number,
    _endTime: number
  ): AudioParamMock {
    this._value = value;
    return this;
  }

  public setTargetAtTime(
    target: number,
    _startTime: number,
    _timeConstant: number
  ): AudioParamMock {
    this._value = target;
    return this;
  }

  public setValueCurveAtTime(
    _values: Float32Array,
    _startTime: number,
    _duration: number
  ): AudioParamMock {
    return this;
  }

  public cancelScheduledValues(_startTime: number): AudioParamMock {
    return this;
  }

  public cancelAndHoldAtTime(_startTime: number): AudioParamMock {
    return this;
  }
}

class AudioBufferMock {
  public sampleRate: number;
  public length: number;
  public duration: number;
  public numberOfChannels: number;

  constructor(options: {
    numberOfChannels: number;
    length: number;
    sampleRate: number;
  }) {
    this.numberOfChannels = options.numberOfChannels;
    this.length = options.length;
    this.sampleRate = options.sampleRate;
    this.duration = options.length / options.sampleRate;
  }

  getChannelData(_channel: number): Float32Array {
    return new Float32Array(this.length);
  }

  copyFromChannel(
    _destination: Float32Array,
    _channelNumber: number,
    _startInChannel?: number
  ): void {}

  copyToChannel(
    _source: Float32Array,
    _channelNumber: number,
    _startInChannel?: number
  ): void {}
}

class AudioNodeMock {
  public context: BaseAudioContextMock;
  public numberOfInputs: number = 1;
  public numberOfOutputs: number = 1;
  public channelCount: number = 2;
  public channelCountMode: ChannelCountMode = 'max';
  public channelInterpretation: ChannelInterpretation = 'speakers';

  constructor(context: BaseAudioContextMock, _node: unknown) {
    this.context = context;
  }

  public connect(
    destination: AudioNodeMock | AudioParamMock
  ): AudioNodeMock | void {
    if (destination instanceof AudioParamMock) {
      return;
    }
    return destination;
  }

  public disconnect(_destination?: AudioNodeMock): void {}
}

class AudioScheduledSourceNodeMock extends AudioNodeMock {
  private _onended: ((event: Event) => void) | null = null;

  constructor(context: BaseAudioContextMock, node: unknown) {
    super(context, node);
  }

  public start(_when: number = 0): void {}
  public stop(_when: number = 0): void {}

  public get onended(): ((event: Event) => void) | null {
    return this._onended;
  }

  public set onended(callback: ((event: Event) => void) | null) {
    this._onended = callback;
  }
}

class AnalyserNodeMock extends AudioNodeMock {
  public fftSize: number = 2048;
  public frequencyBinCount: number = 1024;
  public minDecibels: number = -100;
  public maxDecibels: number = -30;
  public smoothingTimeConstant: number = 0.8;

  constructor(context: BaseAudioContextMock, _options?: AnalyserOptions) {
    super(context, {});
  }

  getByteFrequencyData(_array: Uint8Array): void {}
  getByteTimeDomainData(_array: Uint8Array): void {}
  getFloatFrequencyData(_array: Float32Array): void {}
  getFloatTimeDomainData(_array: Float32Array): void {}
}

class GainNodeMock extends AudioNodeMock {
  readonly gain: AudioParamMock;

  constructor(context: BaseAudioContextMock, _options?: GainOptions) {
    super(context, {});
    this.gain = new AudioParamMock({}, context);
    this.gain.value = 1;
  }
}

class DelayNodeMock extends AudioNodeMock {
  readonly delayTime: AudioParamMock;

  constructor(context: BaseAudioContextMock, _options?: DelayOptions) {
    super(context, {});
    this.delayTime = new AudioParamMock({}, context);
    this.delayTime.maxValue = 1;
  }
}

class BiquadFilterNodeMock extends AudioNodeMock {
  private _type: BiquadFilterType = 'lowpass';
  readonly frequency: AudioParamMock;
  readonly detune: AudioParamMock;
  readonly Q: AudioParamMock;
  readonly gain: AudioParamMock;

  constructor(context: BaseAudioContextMock, _options?: BiquadFilterOptions) {
    super(context, {});
    this.frequency = new AudioParamMock({}, context);
    this.detune = new AudioParamMock({}, context);
    this.Q = new AudioParamMock({}, context);
    this.gain = new AudioParamMock({}, context);

    this.frequency.value = 350;
    this.Q.value = 1;
    this.gain.value = 0;
  }

  get type(): BiquadFilterType {
    return this._type;
  }

  set type(value: BiquadFilterType) {
    this._type = value;
  }

  getFrequencyResponse(
    _frequencyHz: Float32Array,
    _magResponse: Float32Array,
    _phaseResponse: Float32Array
  ): void {}
}

class ConvolverNodeMock extends AudioNodeMock {
  private _buffer: AudioBufferMock | null = null;
  public normalize: boolean = true;

  constructor(context: BaseAudioContextMock, _options?: ConvolverOptions) {
    super(context, {});
  }

  get buffer(): AudioBufferMock | null {
    return this._buffer;
  }

  set buffer(value: AudioBufferMock | null) {
    this._buffer = value;
  }
}

class WaveShaperNodeMock extends AudioNodeMock {
  private _curve: Float32Array | null = null;
  private _oversample: OverSampleType = 'none';

  constructor(context: BaseAudioContextMock, _options?: WaveShaperOptions) {
    super(context, {});
  }

  get curve(): Float32Array | null {
    return this._curve;
  }

  set curve(value: Float32Array | null) {
    this._curve = value;
  }

  get oversample(): OverSampleType {
    return this._oversample;
  }

  set oversample(value: OverSampleType) {
    this._oversample = value;
  }
}

class StereoPannerNodeMock extends AudioNodeMock {
  readonly pan: AudioParamMock;

  constructor(context: BaseAudioContextMock, _options?: StereoPannerOptions) {
    super(context, {});
    this.pan = new AudioParamMock({}, context);
  }
}

class OscillatorNodeMock extends AudioScheduledSourceNodeMock {
  private _type: OscillatorType = 'sine';
  readonly frequency: AudioParamMock;
  readonly detune: AudioParamMock;

  constructor(context: BaseAudioContextMock, _options?: OscillatorOptions) {
    super(context, {});
    this.frequency = new AudioParamMock({}, context);
    this.detune = new AudioParamMock({}, context);
    this.frequency.value = 440;
  }

  get type(): OscillatorType {
    return this._type;
  }

  set type(value: OscillatorType) {
    this._type = value;
  }

  public setPeriodicWave(_wave: PeriodicWaveMock): void {}
}

class ConstantSourceNodeMock extends AudioScheduledSourceNodeMock {
  readonly offset: AudioParamMock;

  constructor(context: BaseAudioContextMock, _options?: ConstantSourceOptions) {
    super(context, {});
    this.offset = new AudioParamMock({}, context);
    this.offset.value = 1;
  }
}

class AudioBufferSourceNodeMock extends AudioScheduledSourceNodeMock {
  private _buffer: AudioBufferMock | null = null;
  private _loop: boolean = false;
  private _loopStart: number = 0;
  private _loopEnd: number = 0;
  readonly playbackRate: AudioParamMock;

  constructor(
    context: BaseAudioContextMock,
    _options?: AudioBufferSourceOptions
  ) {
    super(context, {});
    this.playbackRate = new AudioParamMock({}, context);
    this.playbackRate.value = 1;
  }

  get buffer(): AudioBufferMock | null {
    return this._buffer;
  }

  set buffer(value: AudioBufferMock | null) {
    this._buffer = value;
  }

  get loop(): boolean {
    return this._loop;
  }

  set loop(value: boolean) {
    this._loop = value;
  }

  get loopStart(): number {
    return this._loopStart;
  }

  set loopStart(value: number) {
    this._loopStart = value;
  }

  get loopEnd(): number {
    return this._loopEnd;
  }

  set loopEnd(value: number) {
    this._loopEnd = value;
  }
}

class RecorderAdapterNodeMock extends AudioNodeMock {
  public wasConnected: boolean = false;

  constructor(context: BaseAudioContextMock) {
    super(context, {});
  }

  getNode(): Record<string, unknown> {
    return {};
  }
}

class AudioBufferQueueSourceNodeMock extends AudioScheduledSourceNodeMock {
  private _onBufferEnded: ((event: { bufferId: string }) => void) | null = null;
  private eventEmitter = new MockAudioEventEmitter();

  constructor(
    context: BaseAudioContextMock,
    _options?: BaseAudioBufferSourceOptions
  ) {
    super(context, {});
  }

  enqueueBuffer(_buffer: AudioBufferMock): string {
    return Math.random().toString(36).substr(2, 9);
  }

  dequeueBuffer(_bufferId: string): void {}
  clearBuffers(): void {}
  pause(): void {}

  get onBufferEnded(): ((event: { bufferId: string }) => void) | null {
    return this._onBufferEnded;
  }

  set onBufferEnded(callback: ((event: { bufferId: string }) => void) | null) {
    this._onBufferEnded = callback;
  }
}

class StreamerNodeMock extends AudioScheduledSourceNodeMock {
  private hasBeenSetup: boolean = false;
  private _streamPath: string = '';

  constructor(context: BaseAudioContextMock, options?: StreamerOptions) {
    super(context, {});
    if (options?.streamPath) {
      this.initialize(options.streamPath);
    }
  }

  initialize(streamPath: string): boolean {
    if (this.hasBeenSetup) {
      throw new Error('Node is already setup');
    }
    this._streamPath = streamPath;
    this.hasBeenSetup = true;
    return true;
  }

  get streamPath(): string {
    return this._streamPath;
  }

  pause(): void {}
  resume(): void {}
}

class WorkletNodeMock extends AudioNodeMock {
  constructor(
    context: BaseAudioContextMock,
    _runtime: AudioWorkletRuntime,
    _callback: (audioData: Array<Float32Array>, channelCount: number) => void,
    _bufferLength: number,
    _inputChannelCount: number
  ) {
    super(context, {});
  }
}

class WorkletProcessingNodeMock extends AudioNodeMock {
  constructor(
    context: BaseAudioContextMock,
    _runtime: AudioWorkletRuntime,
    _callback: (
      inputData: Array<Float32Array>,
      outputData: Array<Float32Array>,
      framesToProcess: number,
      currentTime: number
    ) => void
  ) {
    super(context, {});
  }
}

class WorkletSourceNodeMock extends AudioScheduledSourceNodeMock {
  constructor(
    context: BaseAudioContextMock,
    _runtime: AudioWorkletRuntime,
    _callback: (
      audioData: Array<Float32Array>,
      framesToProcess: number,
      currentTime: number,
      startOffset: number
    ) => void
  ) {
    super(context, {});
  }
}

class PeriodicWaveMock {
  constructor(_context: BaseAudioContextMock, _options?: PeriodicWaveOptions) {}
}

class AudioDestinationNodeMock extends AudioNodeMock {
  public maxChannelCount: number = 2;

  constructor(context: BaseAudioContextMock) {
    super(context, {});
    this.numberOfOutputs = 0;
  }
}

class BaseAudioContextMock {
  public destination: AudioDestinationNodeMock;
  private _sampleRate: number = 44100;
  private _currentTime: number = 0;
  protected _state: ContextState = 'running';

  constructor(options?: AudioContextOptions) {
    this.destination = new AudioDestinationNodeMock(this);
    if (options?.sampleRate) {
      this._sampleRate = options.sampleRate;
    }
  }

  get currentTime(): number {
    return this._currentTime;
  }

  get sampleRate(): number {
    return this._sampleRate;
  }

  get state(): ContextState {
    return this._state;
  }

  createBuffer(
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBufferMock {
    return new AudioBufferMock({ numberOfChannels, length, sampleRate });
  }

  createPeriodicWave(
    _real?: Float32Array,
    _imag?: Float32Array,
    _constraints?: { disableNormalization?: boolean }
  ): PeriodicWaveMock {
    return new PeriodicWaveMock(this);
  }

  decodeAudioData(_audioData: ArrayBuffer): Promise<AudioBufferMock> {
    return Promise.resolve(
      new AudioBufferMock({
        numberOfChannels: 2,
        length: 44100,
        sampleRate: 44100,
      })
    );
  }

  createAnalyser(options?: AnalyserOptions): AnalyserNodeMock {
    return new AnalyserNodeMock(this, options);
  }

  createBiquadFilter(options?: BiquadFilterOptions): BiquadFilterNodeMock {
    return new BiquadFilterNodeMock(this, options);
  }

  createBufferSource(
    options?: AudioBufferSourceOptions
  ): AudioBufferSourceNodeMock {
    return new AudioBufferSourceNodeMock(this, options);
  }

  createChannelMerger(_numberOfInputs?: number): AudioNodeMock {
    return new AudioNodeMock(this, {});
  }

  createChannelSplitter(_numberOfOutputs?: number): AudioNodeMock {
    return new AudioNodeMock(this, {});
  }

  createConstantSource(
    options?: ConstantSourceOptions
  ): ConstantSourceNodeMock {
    return new ConstantSourceNodeMock(this, options);
  }

  createConvolver(options?: ConvolverOptions): ConvolverNodeMock {
    return new ConvolverNodeMock(this, options);
  }

  createDelay(options?: DelayOptions): DelayNodeMock {
    return new DelayNodeMock(this, options);
  }

  createGain(options?: GainOptions): GainNodeMock {
    return new GainNodeMock(this, options);
  }

  createOscillator(options?: OscillatorOptions): OscillatorNodeMock {
    return new OscillatorNodeMock(this, options);
  }

  createStereoPanner(options?: StereoPannerOptions): StereoPannerNodeMock {
    return new StereoPannerNodeMock(this, options);
  }

  createWaveShaper(options?: WaveShaperOptions): WaveShaperNodeMock {
    return new WaveShaperNodeMock(this, options);
  }

  createRecorderAdapter(): RecorderAdapterNodeMock {
    return new RecorderAdapterNodeMock(this);
  }

  createBufferQueueSource(
    options?: BaseAudioBufferSourceOptions
  ): AudioBufferQueueSourceNodeMock {
    return new AudioBufferQueueSourceNodeMock(this, options);
  }

  createStreamer(options?: StreamerOptions): StreamerNodeMock {
    return new StreamerNodeMock(this, options);
  }

  createWorkletNode(
    _shareableWorklet: Record<string, unknown>,
    _runOnUI: boolean,
    _bufferLength: number,
    _inputChannelCount: number
  ): WorkletNodeMock {
    return new WorkletNodeMock(this, 'AudioRuntime', noop, 0, 0);
  }

  createWorkletProcessingNode(
    _shareableWorklet: Record<string, unknown>,
    _runOnUI: boolean
  ): WorkletProcessingNodeMock {
    return new WorkletProcessingNodeMock(this, 'AudioRuntime', noop);
  }

  createWorkletSourceNode(
    _shareableWorklet: Record<string, unknown>,
    _runOnUI: boolean
  ): WorkletSourceNodeMock {
    return new WorkletSourceNodeMock(this, 'AudioRuntime', noop);
  }
}

class AudioContextMock extends BaseAudioContextMock {
  constructor(options?: AudioContextOptions) {
    super(options);
  }

  close(): Promise<void> {
    this._state = 'closed';
    return Promise.resolve();
  }

  resume(): Promise<void> {
    this._state = 'running';
    return Promise.resolve();
  }

  suspend(): Promise<void> {
    this._state = 'suspended';
    return Promise.resolve();
  }
}

class OfflineAudioContextMock extends BaseAudioContextMock {
  public length: number;

  constructor(options: OfflineAudioContextOptions) {
    super({ sampleRate: options.sampleRate });
    this.length = options.length;
  }

  startRendering(): Promise<AudioBufferMock> {
    return Promise.resolve(
      new AudioBufferMock({
        numberOfChannels: 2,
        length: this.length,
        sampleRate: this.sampleRate,
      })
    );
  }
}

class AudioRecorderMock {
  private _isRecording: boolean = false;
  private _isPaused: boolean = false;
  private _currentDuration: number = 0;
  private _options: AudioRecorderFileOptions | null = null;
  private isFileOutputEnabled: boolean = false;
  private eventEmitter = new MockAudioEventEmitter();
  private onAudioReadySubscription: MockEventSubscription | null = null;
  private onErrorSubscription: MockEventSubscription | null = null;

  constructor() {}

  enableFileOutput(
    options?: Omit<AudioRecorderFileOptions, 'batchDurationSeconds'>
  ): Result<{ path: string }> {
    this._options = options || {};
    this.isFileOutputEnabled = true;
    return { status: 'success', path: '/mock/path/recordings' };
  }

  get options(): AudioRecorderFileOptions | null {
    return this._options;
  }

  disableFileOutput(): void {
    this._options = null;
    this.isFileOutputEnabled = false;
  }

  start(options?: AudioRecorderStartOptions): Result<{ path: string }> {
    this._isRecording = true;
    this._isPaused = false;
    const path = options?.fileNameOverride || 'recording.m4a';
    return { status: 'success', path };
  }

  stop(): Result<FileInfo> {
    this._isRecording = false;
    this._isPaused = false;
    this._currentDuration = 0;
    return {
      status: 'success',
      path: '/mock/path/recording.m4a',
      size: 12345,
      duration: 5.0,
    };
  }

  pause(): void {
    this._isPaused = true;
  }

  resume(): void {
    this._isPaused = false;
  }

  connect(_node: RecorderAdapterNodeMock): void {
    if (_node.wasConnected) {
      throw new Error('RecorderAdapterNode cannot be connected more than once');
    }
    _node.wasConnected = true;
  }

  disconnect(): void {}

  onAudioReady(
    _options: AudioRecorderCallbackOptions,
    callback: (event: Partial<Record<string, unknown>> | undefined) => void
  ): Result<{}> {
    if (this.onAudioReadySubscription) {
      this.onAudioReadySubscription.remove();
    }

    this.onAudioReadySubscription = this.eventEmitter.addAudioEventListener(
      'audioReady',
      callback
    );

    return { status: 'success' };
  }

  clearOnAudioReady(): void {
    if (this.onAudioReadySubscription) {
      this.onAudioReadySubscription.remove();
      this.onAudioReadySubscription = null;
    }
  }

  isRecording(): boolean {
    return this._isRecording;
  }

  isPaused(): boolean {
    return this._isPaused;
  }

  getCurrentDuration(): number {
    return this._currentDuration;
  }

  onError(
    callback: (error: Record<string, unknown> | undefined) => void
  ): void {
    if (this.onErrorSubscription) {
      this.onErrorSubscription.remove();
    }

    this.onErrorSubscription = this.eventEmitter.addAudioEventListener(
      'recorderError',
      callback
    );
  }

  clearOnError(): void {
    if (this.onErrorSubscription) {
      this.onErrorSubscription.remove();
      this.onErrorSubscription = null;
    }
  }
}

const decodeAudioData = (_audioData: ArrayBuffer): Promise<AudioBufferMock> => {
  return Promise.resolve(
    new AudioBufferMock({
      numberOfChannels: 2,
      length: 44100,
      sampleRate: 44100,
    })
  );
};

const decodePCMInBase64 = (_base64Data: string): Promise<AudioBufferMock> => {
  return Promise.resolve(
    new AudioBufferMock({
      numberOfChannels: 2,
      length: 44100,
      sampleRate: 44100,
    })
  );
};

const changePlaybackSpeed = (
  buffer: AudioBufferMock,
  _speed: number
): Promise<AudioBufferMock> => {
  return Promise.resolve(buffer);
};

class AudioManagerMock {
  static getDevicePreferredSampleRate(): number {
    return 44100;
  }

  static observeVolumeChanges(_observe: boolean): void {}

  static addSystemEventListener(
    _event: string,
    _callback: (event: { value: number }) => void
  ): { remove: () => void } {
    return { remove: noop };
  }

  static removeSystemEventListener(_listener: { remove: () => void }): void {}
}

class NotificationManagerMock {
  static create(_options: Record<string, unknown>): {
    update: () => void;
    destroy: () => void;
  } {
    return {
      update: noop,
      destroy: noop,
    };
  }
}

class PlaybackNotificationManagerMock {
  static create(_options: Record<string, unknown>): {
    update: () => void;
    destroy: () => void;
  } {
    return {
      update: noop,
      destroy: noop,
    };
  }
}

class RecordingNotificationManagerMock {
  static create(_options: Record<string, unknown>): {
    update: () => void;
    destroy: () => void;
  } {
    return {
      update: noop,
      destroy: noop,
    };
  }
}

let mockSystemVolumeValue = 0.5;
const useSystemVolume = (): number => mockSystemVolumeValue;
const setMockSystemVolume = (volume: number): void => {
  mockSystemVolumeValue = volume;
};

class FilePresetMock {
  static get Low(): FilePresetType {
    return {} as FilePresetType;
  }

  static get Medium(): FilePresetType {
    return {} as FilePresetType;
  }

  static get High(): FilePresetType {
    return {} as FilePresetType;
  }

  static get Lossless(): FilePresetType {
    return {} as FilePresetType;
  }
}

class NotSupportedErrorMock extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'NotSupportedError';
  }
}

class InvalidAccessErrorMock extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'InvalidAccessError';
  }
}

class InvalidStateErrorMock extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'InvalidStateError';
  }
}

class IndexSizeErrorMock extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'IndexSizeError';
  }
}

class RangeErrorMock extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'RangeError';
  }
}

class AudioApiErrorMock extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'AudioApiError';
  }
}

// Export classes with original API names (for compatibility)
export const AnalyserNode = AnalyserNodeMock;
export const AudioBuffer = AudioBufferMock;
export const AudioBufferQueueSourceNode = AudioBufferQueueSourceNodeMock;
export const AudioBufferSourceNode = AudioBufferSourceNodeMock;
export const AudioContext = AudioContextMock;
export const AudioDestinationNode = AudioDestinationNodeMock;
export const AudioNode = AudioNodeMock;
export const AudioParam = AudioParamMock;
export const AudioRecorder = AudioRecorderMock;
export const AudioScheduledSourceNode = AudioScheduledSourceNodeMock;
export const BaseAudioContext = BaseAudioContextMock;
export const BiquadFilterNode = BiquadFilterNodeMock;
export const ConstantSourceNode = ConstantSourceNodeMock;
export const ConvolverNode = ConvolverNodeMock;
export const DelayNode = DelayNodeMock;
export const GainNode = GainNodeMock;
export const OfflineAudioContext = OfflineAudioContextMock;
export const OscillatorNode = OscillatorNodeMock;
export const RecorderAdapterNode = RecorderAdapterNodeMock;
export const StereoPannerNode = StereoPannerNodeMock;
export const StreamerNode = StreamerNodeMock;
export const WaveShaperNode = WaveShaperNodeMock;
export const WorkletNode = WorkletNodeMock;
export const WorkletProcessingNode = WorkletProcessingNodeMock;
export const WorkletSourceNode = WorkletSourceNodeMock;
export const PeriodicWave = PeriodicWaveMock;

export const AudioManager = AudioManagerMock;
export const NotificationManager = NotificationManagerMock;
export const PlaybackNotificationManager = PlaybackNotificationManagerMock;
export const RecordingNotificationManager = RecordingNotificationManagerMock;

export const FilePreset = FilePresetMock;

export const NotSupportedError = NotSupportedErrorMock;
export const InvalidAccessError = InvalidAccessErrorMock;
export const InvalidStateError = InvalidStateErrorMock;
export const IndexSizeError = IndexSizeErrorMock;
export const RangeError = RangeErrorMock;
export const AudioApiError = AudioApiErrorMock;

// Export functions
export {
  changePlaybackSpeed,
  decodeAudioData,
  decodePCMInBase64,
  setMockSystemVolume,
  useSystemVolume,
};

// Type exports to allow using classes as types
export type AnalyserNode = AnalyserNodeMock;
export type AudioBuffer = AudioBufferMock;
export type AudioBufferQueueSourceNode = AudioBufferQueueSourceNodeMock;
export type AudioBufferSourceNode = AudioBufferSourceNodeMock;
export type AudioContext = AudioContextMock;
export type AudioDestinationNode = AudioDestinationNodeMock;
export type AudioNode = AudioNodeMock;
export type AudioParam = AudioParamMock;
export type AudioRecorder = AudioRecorderMock;
export type AudioScheduledSourceNode = AudioScheduledSourceNodeMock;
export type BaseAudioContext = BaseAudioContextMock;
export type BiquadFilterNode = BiquadFilterNodeMock;
export type ConstantSourceNode = ConstantSourceNodeMock;
export type ConvolverNode = ConvolverNodeMock;
export type DelayNode = DelayNodeMock;
export type GainNode = GainNodeMock;
export type OfflineAudioContext = OfflineAudioContextMock;
export type OscillatorNode = OscillatorNodeMock;
export type RecorderAdapterNode = RecorderAdapterNodeMock;
export type StereoPannerNode = StereoPannerNodeMock;
export type StreamerNode = StreamerNodeMock;
export type WaveShaperNode = WaveShaperNodeMock;
export type WorkletNode = WorkletNodeMock;
export type WorkletProcessingNode = WorkletProcessingNodeMock;
export type WorkletSourceNode = WorkletSourceNodeMock;
export type PeriodicWave = PeriodicWaveMock;

// Export types and enums
export {
  AudioContextOptions,
  AudioRecorderCallbackOptions,
  AudioRecorderFileOptions,
  AudioRecorderStartOptions,
  AudioWorkletRuntime,
  BiquadFilterType,
  ChannelCountMode,
  ChannelInterpretation,
  ContextState,
  FileDirectory,
  FileFormat,
  FileInfo,
  FilePresetType,
  OfflineAudioContextOptions,
  OscillatorType,
  OverSampleType,
  Result,
  AnalyserOptions,
  AudioBufferSourceOptions,
  BaseAudioBufferSourceOptions,
  BiquadFilterOptions,
  ConstantSourceOptions,
  ConvolverOptions,
  DelayOptions,
  GainOptions,
  OscillatorOptions,
  PeriodicWaveOptions,
  StereoPannerOptions,
  StreamerOptions,
  WaveShaperOptions,
};

export default {
  AnalyserNode: AnalyserNodeMock,
  AudioBuffer: AudioBufferMock,
  AudioBufferQueueSourceNode: AudioBufferQueueSourceNodeMock,
  AudioBufferSourceNode: AudioBufferSourceNodeMock,
  AudioContext: AudioContextMock,
  AudioDestinationNode: AudioDestinationNodeMock,
  AudioNode: AudioNodeMock,
  AudioParam: AudioParamMock,
  AudioRecorder: AudioRecorderMock,
  AudioScheduledSourceNode: AudioScheduledSourceNodeMock,
  BaseAudioContext: BaseAudioContextMock,
  BiquadFilterNode: BiquadFilterNodeMock,
  ConstantSourceNode: ConstantSourceNodeMock,
  ConvolverNode: ConvolverNodeMock,
  DelayNode: DelayNodeMock,
  GainNode: GainNodeMock,
  OfflineAudioContext: OfflineAudioContextMock,
  OscillatorNode: OscillatorNodeMock,
  RecorderAdapterNode: RecorderAdapterNodeMock,
  StereoPannerNode: StereoPannerNodeMock,
  StreamerNode: StreamerNodeMock,
  WaveShaperNode: WaveShaperNodeMock,
  WorkletNode: WorkletNodeMock,
  WorkletProcessingNode: WorkletProcessingNodeMock,
  WorkletSourceNode: WorkletSourceNodeMock,
  PeriodicWave: PeriodicWaveMock,

  // Functions
  decodeAudioData,
  decodePCMInBase64,
  changePlaybackSpeed,
  useSystemVolume,
  setMockSystemVolume,

  // System classes
  AudioManager: AudioManagerMock,
  NotificationManager: NotificationManagerMock,
  PlaybackNotificationManager: PlaybackNotificationManagerMock,
  RecordingNotificationManager: RecordingNotificationManagerMock,

  // Utils
  FilePreset: FilePresetMock,

  // Errors
  NotSupportedError: NotSupportedErrorMock,
  InvalidAccessError: InvalidAccessErrorMock,
  InvalidStateError: InvalidStateErrorMock,
  IndexSizeError: IndexSizeErrorMock,
  RangeError: RangeErrorMock,
  AudioApiError: AudioApiErrorMock,

  // Enums
  FileDirectory,
  FileFormat,
};

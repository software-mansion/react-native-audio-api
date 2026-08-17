import {
  AudioContextOptions,
  AudioRecorderCallbackOptions,
  AudioRecorderFileOptions,
  AudioRecorderOptions,
  AudioRecorderStartOptions,
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
  AudioBufferQueueSourceOptions,
  BiquadFilterOptions,
  ChannelMergerOptions,
  ChannelSplitterOptions,
  ConstantSourceOptions,
  ConvolverOptions,
  DelayOptions,
  DecodeDataInput,
  GainOptions,
  OscillatorOptions,
  PeriodicWaveOptions,
  StereoPannerOptions,
  WaveShaperOptions,
} from '../types';
import { toFloat32Array } from '../utils';

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
    destination: AudioNodeMock | AudioParamMock,
    output: number = 0,
    input: number = 0
  ): AudioNodeMock | void {
    if (output < 0 || output >= this.numberOfOutputs) {
      throw new IndexSizeErrorMock(
        `The output index provided (${output}) is outside the range [0, ${this.numberOfOutputs})`
      );
    }

    if (destination instanceof AudioParamMock) {
      return;
    }

    if (input < 0 || input >= destination.numberOfInputs) {
      throw new IndexSizeErrorMock(
        `The input index provided (${input}) is outside the range [0, ${destination.numberOfInputs})`
      );
    }

    return destination;
  }

  public disconnect(
    destinationOrOutput?: AudioNodeMock | AudioParamMock | number,
    output?: number,
    input?: number
  ): void {
    if (typeof destinationOrOutput === 'number') {
      if (
        destinationOrOutput < 0 ||
        destinationOrOutput >= this.numberOfOutputs
      ) {
        throw new IndexSizeErrorMock(
          `The output index provided (${destinationOrOutput}) is outside the range [0, ${this.numberOfOutputs})`
        );
      }
      return;
    }

    if (
      output !== undefined &&
      (output < 0 || output >= this.numberOfOutputs)
    ) {
      throw new IndexSizeErrorMock(
        `The output index provided (${output}) is outside the range [0, ${this.numberOfOutputs})`
      );
    }

    if (
      destinationOrOutput instanceof AudioNodeMock &&
      input !== undefined &&
      (input < 0 || input >= destinationOrOutput.numberOfInputs)
    ) {
      throw new IndexSizeErrorMock(
        `The input index provided (${input}) is outside the range [0, ${destinationOrOutput.numberOfInputs})`
      );
    }
  }
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

class ChannelMergerNodeMock extends AudioNodeMock {
  constructor(context: BaseAudioContextMock, options?: ChannelMergerOptions) {
    super(context, {});

    if (options?.numberOfInputs !== undefined) {
      const { numberOfInputs } = options;
      if (
        !Number.isInteger(numberOfInputs) ||
        numberOfInputs < 1 ||
        numberOfInputs > 32
      ) {
        throw new IndexSizeErrorMock(
          `The numberOfInputs value (${numberOfInputs}) is outside the range [1, 32]`
        );
      }
    }

    if (options?.channelCount !== undefined && options.channelCount !== 1) {
      throw new InvalidStateErrorMock(
        `ChannelMergerNode channelCount cannot be changed from 1`
      );
    }

    if (
      options?.channelCountMode !== undefined &&
      options.channelCountMode !== 'explicit'
    ) {
      throw new InvalidStateErrorMock(
        `ChannelMergerNode channelCountMode cannot be changed from 'explicit'`
      );
    }

    this.numberOfInputs = options?.numberOfInputs ?? 6;
    this.numberOfOutputs = 1;
    this.channelCount = 1;
    this.channelCountMode = 'explicit';
    this.channelInterpretation = options?.channelInterpretation ?? 'speakers';
  }
}

class ChannelSplitterNodeMock extends AudioNodeMock {
  constructor(context: BaseAudioContextMock, options?: ChannelSplitterOptions) {
    super(context, {});
    const numberOfOutputs = options?.numberOfOutputs ?? 6;

    if (options?.numberOfOutputs !== undefined) {
      if (
        !Number.isInteger(numberOfOutputs) ||
        numberOfOutputs < 1 ||
        numberOfOutputs > 32
      ) {
        throw new IndexSizeErrorMock(
          `The numberOfOutputs value (${numberOfOutputs}) is outside the range [1, 32]`
        );
      }
    }

    if (
      options?.channelCount !== undefined &&
      options.channelCount !== numberOfOutputs
    ) {
      throw new InvalidStateErrorMock(
        `ChannelSplitterNode channelCount cannot be changed from ${numberOfOutputs}`
      );
    }

    if (
      options?.channelCountMode !== undefined &&
      options.channelCountMode !== 'explicit'
    ) {
      throw new InvalidStateErrorMock(
        `ChannelSplitterNode channelCountMode cannot be changed from 'explicit'`
      );
    }

    if (
      options?.channelInterpretation !== undefined &&
      options.channelInterpretation !== 'discrete'
    ) {
      throw new InvalidStateErrorMock(
        `ChannelSplitterNode channelInterpretation cannot be changed from 'discrete'`
      );
    }

    this.numberOfInputs = 1;
    this.numberOfOutputs = numberOfOutputs;
    this.channelCount = numberOfOutputs;
    this.channelCountMode = 'explicit';
    this.channelInterpretation = 'discrete';
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
  private curveWasSet = false;

  constructor(context: BaseAudioContextMock, options?: WaveShaperOptions) {
    super(context, {});
    if (options?.curve) {
      this._curve = toFloat32Array(options.curve);
      this.curveWasSet = true;
    }
    if (options?.oversample) {
      this._oversample = options.oversample;
    }
  }

  get curve(): Float32Array | null {
    return this._curve;
  }

  set curve(value: Float32Array | null) {
    if (value !== null) {
      if (this.curveWasSet) {
        throw new InvalidStateErrorMock(
          'The curve can only be set once and cannot be changed afterwards.'
        );
      }
      if (value.length < 2) {
        throw new InvalidStateErrorMock(
          'The curve must have at least two values if not null.'
        );
      }
      this.curveWasSet = true;
    }
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
    _options?: AudioBufferQueueSourceOptions
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

class MediaElementAudioSourceNodeMock extends AudioNodeMock {
  readonly mediaElement: HTMLMediaElement | AudioNodeMock;

  constructor(
    context: BaseAudioContextMock,
    mediaElement: HTMLMediaElement | AudioNodeMock
  ) {
    super(context, {});
    this.mediaElement = mediaElement;
    this.numberOfInputs = 0;
  }
}

class PeriodicWaveMock {
  constructor(_context: BaseAudioContextMock, _options?: PeriodicWaveOptions) {}
}

class AudioDestinationNodeMock extends AudioNodeMock {
  public maxChannelCount: number = 2;

  constructor(context: BaseAudioContextMock) {
    super(context, {});
    this.numberOfOutputs = 1;
  }
}

class AudioListenerMock {
  public positionX: AudioParamMock;
  public positionY: AudioParamMock;
  public positionZ: AudioParamMock;
  public forwardX: AudioParamMock;
  public forwardY: AudioParamMock;
  public forwardZ: AudioParamMock;
  public upX: AudioParamMock;
  public upY: AudioParamMock;
  public upZ: AudioParamMock;

  constructor(context: BaseAudioContextMock) {
    this.positionX = new AudioParamMock(null, context);
    this.positionY = new AudioParamMock(null, context);
    this.positionZ = new AudioParamMock(null, context);
    this.forwardX = new AudioParamMock(null, context);
    this.forwardY = new AudioParamMock(null, context);
    this.forwardZ = new AudioParamMock(null, context);
    this.forwardZ.value = -1;
    this.upX = new AudioParamMock(null, context);
    this.upY = new AudioParamMock(null, context);
    this.upY.value = 1;
    this.upZ = new AudioParamMock(null, context);
  }
}

class BaseAudioContextMock {
  public destination: AudioDestinationNodeMock;
  public listener: AudioListenerMock;
  private _sampleRate: number = 44100;
  private _currentTime: number = 0;
  protected _state: ContextState = 'running';

  constructor(options?: AudioContextOptions) {
    this.destination = new AudioDestinationNodeMock(this);
    this.listener = new AudioListenerMock(this);
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

  get baseLatency(): number {
    return 0.005;
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

  createChannelMerger(numberOfInputs?: number): ChannelMergerNodeMock {
    return new ChannelMergerNodeMock(
      this,
      numberOfInputs !== undefined ? { numberOfInputs } : undefined
    );
  }

  createChannelSplitter(numberOfOutputs?: number): ChannelSplitterNodeMock {
    return new ChannelSplitterNodeMock(
      this,
      numberOfOutputs !== undefined ? { numberOfOutputs } : undefined
    );
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
    options?: AudioBufferQueueSourceOptions
  ): AudioBufferQueueSourceNodeMock {
    return new AudioBufferQueueSourceNodeMock(this, options);
  }
}

class AudioContextMock extends BaseAudioContextMock {
  constructor(options?: AudioContextOptions) {
    super(options);
  }

  get outputLatency(): number {
    return 0.01;
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

  createMediaElementSource(
    mediaElement: HTMLMediaElement | AudioNodeMock
  ): MediaElementAudioSourceNodeMock {
    return new MediaElementAudioSourceNodeMock(this, mediaElement);
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
  private static lastCreated: AudioRecorderMock | null = null;

  private _isRecording: boolean = false;
  private _isPaused: boolean = false;
  private _currentDuration: number = 0;
  private _options: AudioRecorderFileOptions | null = null;
  private isFileOutputEnabled: boolean = false;
  private eventEmitter = new MockAudioEventEmitter();
  private onAudioReadySubscription: MockEventSubscription | null = null;
  private onErrorSubscription: MockEventSubscription | null = null;

  // Options only configure the native capture chain, so the mock ignores them.
  constructor(_options?: AudioRecorderOptions) {
    AudioRecorderMock.lastCreated = this;
  }

  static isRecordingOngoing(): boolean {
    const recorder = AudioRecorderMock.lastCreated;
    return recorder != null && (recorder._isRecording || recorder._isPaused);
  }

  static takeLastRecordingResult(): FileInfo | null {
    return null;
  }

  enableFileOutput(
    options?: AudioRecorderFileOptions
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

  start(
    options?: AudioRecorderStartOptions
  ): Promise<Result<{ path: string }>> {
    this._isRecording = true;
    this._isPaused = false;
    const path = options?.fileNameOverride || 'recording.m4a';
    return Promise.resolve({ status: 'success', path });
  }

  stop(): Promise<Result<FileInfo>> {
    this._isRecording = false;
    this._isPaused = false;
    this._currentDuration = 0;
    return Promise.resolve({
      status: 'success',
      paths: ['/mock/path/recording.m4a'],
      size: 12345,
      duration: 5.0,
    });
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

  getInputLatency(): number {
    return 0.01;
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

const getAudioDuration = (_input: DecodeDataInput): Promise<number> => {
  if (_input instanceof ArrayBuffer) {
    return Promise.resolve(1);
  }

  if (
    typeof _input === 'string' &&
    _input.startsWith('data:audio/') &&
    _input.includes(';base64,')
  ) {
    return Promise.reject(
      new AudioApiErrorMock(
        'Base64 source decoding is not currently supported, to decode raw PCM base64 strings use decodePCMInBase64 method.'
      )
    );
  }

  if (typeof _input === 'string' && _input.startsWith('blob:')) {
    return Promise.reject(
      new AudioApiErrorMock(
        'Data Blob string decoding is not currently supported.'
      )
    );
  }

  return Promise.resolve(1);
};

const changePlaybackSpeed = (
  buffer: AudioBufferMock,
  _speed: number
): Promise<AudioBufferMock> => {
  return Promise.resolve(buffer);
};

const concatAudioFiles = (
  inputPaths: string[],
  outputPath: string
): Promise<string> => {
  if (!Array.isArray(inputPaths) || inputPaths.length === 0) {
    return Promise.reject(
      new AudioApiErrorMock(
        'concatAudioFiles requires at least one input path.'
      )
    );
  }

  if (inputPaths.some((inputPath) => typeof inputPath !== 'string')) {
    return Promise.reject(
      new TypeError('concatAudioFiles input paths must be strings.')
    );
  }

  if (typeof outputPath !== 'string' || outputPath.length === 0) {
    return Promise.reject(
      new AudioApiErrorMock('concatAudioFiles requires an output path.')
    );
  }

  return Promise.resolve(outputPath);
};

const isFfmpegEnabled = (): boolean => true;

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
export const AudioListener = AudioListenerMock;
export const AudioNode = AudioNodeMock;
export const AudioParam = AudioParamMock;
export const AudioRecorder = AudioRecorderMock;
export const AudioScheduledSourceNode = AudioScheduledSourceNodeMock;
export const BaseAudioContext = BaseAudioContextMock;
export const BiquadFilterNode = BiquadFilterNodeMock;
export const ChannelMergerNode = ChannelMergerNodeMock;
export const ChannelSplitterNode = ChannelSplitterNodeMock;
export const ConstantSourceNode = ConstantSourceNodeMock;
export const ConvolverNode = ConvolverNodeMock;
export const DelayNode = DelayNodeMock;
export const GainNode = GainNodeMock;
export const MediaElementAudioSourceNode = MediaElementAudioSourceNodeMock;
export const OfflineAudioContext = OfflineAudioContextMock;
export const OscillatorNode = OscillatorNodeMock;
export const RecorderAdapterNode = RecorderAdapterNodeMock;
export const StereoPannerNode = StereoPannerNodeMock;
export const WaveShaperNode = WaveShaperNodeMock;
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
  concatAudioFiles,
  decodeAudioData,
  decodePCMInBase64,
  getAudioDuration,
  isFfmpegEnabled,
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
export type AudioListener = AudioListenerMock;
export type AudioNode = AudioNodeMock;
export type AudioParam = AudioParamMock;
export type AudioRecorder = AudioRecorderMock;
export type AudioScheduledSourceNode = AudioScheduledSourceNodeMock;
export type BaseAudioContext = BaseAudioContextMock;
export type BiquadFilterNode = BiquadFilterNodeMock;
export type ChannelMergerNode = ChannelMergerNodeMock;
export type ChannelSplitterNode = ChannelSplitterNodeMock;
export type ConstantSourceNode = ConstantSourceNodeMock;
export type ConvolverNode = ConvolverNodeMock;
export type DelayNode = DelayNodeMock;
export type GainNode = GainNodeMock;
export type MediaElementAudioSourceNode = MediaElementAudioSourceNodeMock;
export type OfflineAudioContext = OfflineAudioContextMock;
export type OscillatorNode = OscillatorNodeMock;
export type RecorderAdapterNode = RecorderAdapterNodeMock;
export type StereoPannerNode = StereoPannerNodeMock;
export type WaveShaperNode = WaveShaperNodeMock;
export type PeriodicWave = PeriodicWaveMock;

// Export types and enums
export {
  AudioContextOptions,
  AudioRecorderCallbackOptions,
  AudioRecorderFileOptions,
  AudioRecorderStartOptions,
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
  AudioBufferQueueSourceOptions,
  BiquadFilterOptions,
  ConstantSourceOptions,
  ConvolverOptions,
  DelayOptions,
  GainOptions,
  OscillatorOptions,
  PeriodicWaveOptions,
  StereoPannerOptions,
  WaveShaperOptions,
};

export default {
  AnalyserNode: AnalyserNodeMock,
  AudioBuffer: AudioBufferMock,
  AudioBufferQueueSourceNode: AudioBufferQueueSourceNodeMock,
  AudioBufferSourceNode: AudioBufferSourceNodeMock,
  AudioContext: AudioContextMock,
  AudioDestinationNode: AudioDestinationNodeMock,
  AudioListener: AudioListenerMock,
  AudioNode: AudioNodeMock,
  AudioParam: AudioParamMock,
  AudioRecorder: AudioRecorderMock,
  AudioScheduledSourceNode: AudioScheduledSourceNodeMock,
  BaseAudioContext: BaseAudioContextMock,
  BiquadFilterNode: BiquadFilterNodeMock,
  ChannelMergerNode: ChannelMergerNodeMock,
  ChannelSplitterNode: ChannelSplitterNodeMock,
  ConstantSourceNode: ConstantSourceNodeMock,
  ConvolverNode: ConvolverNodeMock,
  DelayNode: DelayNodeMock,
  GainNode: GainNodeMock,
  MediaElementAudioSourceNode: MediaElementAudioSourceNodeMock,
  OfflineAudioContext: OfflineAudioContextMock,
  OscillatorNode: OscillatorNodeMock,
  RecorderAdapterNode: RecorderAdapterNodeMock,
  StereoPannerNode: StereoPannerNodeMock,
  WaveShaperNode: WaveShaperNodeMock,
  PeriodicWave: PeriodicWaveMock,

  // Functions
  decodeAudioData,
  decodePCMInBase64,
  getAudioDuration,
  changePlaybackSpeed,
  concatAudioFiles,
  isFfmpegEnabled,
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

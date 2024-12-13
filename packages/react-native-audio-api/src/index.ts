import { ContextState, PeriodicWaveConstraints } from './core/types';

export class AudioBuffer {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;

  /** @internal */
  public readonly buffer: globalThis.AudioBuffer;

  constructor(buffer: globalThis.AudioBuffer) {
    this.buffer = buffer;
    this.length = buffer.length;
    this.duration = buffer.duration;
    this.sampleRate = buffer.sampleRate;
    this.numberOfChannels = buffer.numberOfChannels;
  }

  public getChannelData(channel: number): number[] {
    if (channel < 0 || channel >= this.numberOfChannels) {
      throw new Error(
        `The channel number provided (${channel}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    // TODO: we probably will want to move to Float32Array on native side too
    return Array.from(this.buffer.getChannelData(channel));
  }

  public copyFromChannel(
    destination: number[],
    channelNumber: number,
    startInChannel: number = 0
  ): void {
    if (channelNumber < 0 || channelNumber >= this.numberOfChannels) {
      throw new Error(
        `The channel number provided (${channelNumber}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    if (startInChannel < 0 || startInChannel >= this.length) {
      throw new Error(
        `The startInChannel number provided (${startInChannel}) is outside the range [0, ${this.length - 1}]`
      );
    }

    this.buffer.copyFromChannel(
      new Float32Array(destination),
      channelNumber,
      startInChannel
    );
  }

  public copyToChannel(
    source: number[],
    channelNumber: number,
    startInChannel: number = 0
  ): void {
    if (channelNumber < 0 || channelNumber >= this.numberOfChannels) {
      throw new Error(
        `The channel number provided (${channelNumber}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    if (startInChannel < 0 || startInChannel >= this.length) {
      throw new Error(
        `The startInChannel number provided (${startInChannel}) is outside the range [0, ${this.length - 1}]`
      );
    }

    this.buffer.copyToChannel(
      new Float32Array(source),
      channelNumber,
      startInChannel
    );
  }
}

export class AudioNode {
  readonly context: AudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;
  readonly channelCount: number;
  readonly channelCountMode: ChannelCountMode;
  readonly channelInterpretation: ChannelInterpretation;

  protected readonly node: globalThis.AudioNode;

  constructor(context: AudioContext, node: globalThis.AudioNode) {
    this.context = context;
    this.node = node;
    this.numberOfInputs = this.node.numberOfInputs;
    this.numberOfOutputs = this.node.numberOfOutputs;
    this.channelCount = this.node.channelCount;
    this.channelCountMode = this.node.channelCountMode;
    this.channelInterpretation = this.node.channelInterpretation;
  }

  public connect(node: AudioNode): void {
    if (this.context !== node.context) {
      throw new Error('The AudioNodes are from different BaseAudioContexts');
    }

    this.node.connect(node.node);
  }

  public disconnect(node: AudioNode): void {
    this.node.disconnect(node.node);
  }
}

export class AudioScheduledSourceNode extends AudioNode {
  private hasBeenStarted: boolean = false;

  constructor(
    context: AudioContext,
    node: globalThis.AudioScheduledSourceNode
  ) {
    super(context, node);
  }

  public start(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${when}`
      );
    }

    if (this.hasBeenStarted) {
      throw new Error('Cannot call start more than once');
    }

    this.hasBeenStarted = true;
    (this.node as globalThis.AudioScheduledSourceNode).start(when);
  }

  public stop(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${when}`
      );
    }

    if (!this.hasBeenStarted) {
      throw new Error('Cannot call stop without calling start first');
    }

    (this.node as globalThis.AudioScheduledSourceNode).stop(when);
  }
}

export class AudioBufferSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  constructor(context: AudioContext, node: globalThis.AudioBufferSourceNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune);
    this.playbackRate = new AudioParam(node.playbackRate);
  }

  public get buffer(): AudioBuffer | null {
    const buffer = (this.node as globalThis.AudioBufferSourceNode).buffer;

    if (!buffer) {
      return null;
    }

    return new AudioBuffer(buffer);
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      (this.node as globalThis.AudioBufferSourceNode | null) = null;
      return;
    }

    (this.node as globalThis.AudioBufferSourceNode).buffer = buffer.buffer;
  }

  public get loop(): boolean {
    return (this.node as globalThis.AudioBufferSourceNode).loop;
  }

  public set loop(value: boolean) {
    (this.node as globalThis.AudioBufferSourceNode).loop = value;
  }

  public get loopStart(): number {
    return (this.node as globalThis.AudioBufferSourceNode).loopStart;
  }

  public set loopStart(value: number) {
    (this.node as globalThis.AudioBufferSourceNode).loopStart = value;
  }

  public get loopEnd(): number {
    return (this.node as globalThis.AudioBufferSourceNode).loopEnd;
  }

  public set loopEnd(value: number) {
    (this.node as globalThis.AudioBufferSourceNode).loopEnd = value;
  }
}

export class AudioDestinationNode extends AudioNode {
  constructor(context: AudioContext, node: globalThis.AudioDestinationNode) {
    super(context, node);
  }
}

export class AudioParam {
  readonly defaultValue: number;
  readonly minValue: number;
  readonly maxValue: number;

  private readonly param: globalThis.AudioParam;

  constructor(param: globalThis.AudioParam) {
    this.param = param;
    this.defaultValue = param.defaultValue;
    this.minValue = param.minValue;
    this.maxValue = param.maxValue;
  }

  public get value(): number {
    return this.param.value;
  }

  public set value(value: number) {
    this.param.value = value;
  }

  public setValueAtTime(value: number, startTime: number): void {
    this.param.setValueAtTime(value, startTime);
  }

  public linearRampToValueAtTime(value: number, endTime: number): void {
    this.param.linearRampToValueAtTime(value, endTime);
  }

  public exponentialRampToValueAtTime(value: number, endTime: number): void {
    this.param.exponentialRampToValueAtTime(value, endTime);
  }
}

export class BiquadFilterNode extends AudioNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;
  readonly Q: AudioParam;
  readonly gain: AudioParam;

  constructor(
    context: AudioContext,
    biquadFilter: globalThis.BiquadFilterNode
  ) {
    super(context, biquadFilter);
    this.frequency = new AudioParam(biquadFilter.frequency);
    this.detune = new AudioParam(biquadFilter.detune);
    this.Q = new AudioParam(biquadFilter.Q);
    this.gain = new AudioParam(biquadFilter.gain);
  }

  public get type(): BiquadFilterType {
    return (this.node as globalThis.BiquadFilterNode).type;
  }

  public set type(value: BiquadFilterType) {
    (this.node as globalThis.BiquadFilterNode).type = value;
  }

  public getFrequencyResponse(
    frequencyArray: number[],
    magResponseOutput: number[],
    phaseResponseOutput: number[]
  ) {
    if (
      frequencyArray.length !== magResponseOutput.length ||
      frequencyArray.length !== phaseResponseOutput.length
    ) {
      throw new Error(
        `The lengths of the arrays are not the same frequencyArray: ${frequencyArray.length}, magResponseOutput: ${magResponseOutput.length}, phaseResponseOutput: ${phaseResponseOutput.length}`
      );
    }

    (this.node as globalThis.BiquadFilterNode).getFrequencyResponse(
      new Float32Array(frequencyArray),
      new Float32Array(magResponseOutput),
      new Float32Array(phaseResponseOutput)
    );
  }
}

export class PeriodicWave {
  readonly periodicWave: globalThis.PeriodicWave;

  constructor(periodicWave: globalThis.PeriodicWave) {
    this.periodicWave = periodicWave;
  }
}

export class OscillatorNode extends AudioScheduledSourceNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;

  constructor(context: AudioContext, node: globalThis.OscillatorNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune);
    this.frequency = new AudioParam(node.frequency);
  }

  public get type(): OscillatorType {
    return (this.node as globalThis.OscillatorNode).type;
  }

  public set type(value: OscillatorType) {
    if (value === 'custom') {
      throw new Error(
        "'type' cannot be set directly to 'custom'.  Use setPeriodicWave() to create a custom Oscillator type."
      );
    }

    (this.node as globalThis.OscillatorNode).type = value;
  }

  public setPeriodicWave(wave: PeriodicWave): void {
    (this.node as globalThis.OscillatorNode).setPeriodicWave(wave.periodicWave);
  }
}

export class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: AudioContext, gain: globalThis.GainNode) {
    super(context, gain);
    this.gain = new AudioParam(gain.gain);
  }
}

export class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(context: AudioContext, pan: globalThis.StereoPannerNode) {
    super(context, pan);
    this.pan = new AudioParam(pan.pan);
  }
}

export class AudioContext {
  protected readonly context: globalThis.AudioContext;

  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  constructor() {
    this.context = new window.AudioContext();

    this.sampleRate = this.context.sampleRate;
    this.destination = new AudioDestinationNode(this, this.context.destination);
  }

  public get currentTime(): number {
    return this.context.currentTime;
  }

  public get state(): ContextState {
    return this.context.state as ContextState;
  }

  createOscillator(): OscillatorNode {
    return new OscillatorNode(this, this.context.createOscillator());
  }

  createGain(): GainNode {
    return new GainNode(this, this.context.createGain());
  }

  createStereoPanner(): StereoPannerNode {
    return new StereoPannerNode(this, this.context.createStereoPanner());
  }

  createBiquadFilter(): BiquadFilterNode {
    return new BiquadFilterNode(this, this.context.createBiquadFilter());
  }

  createBufferSource(): AudioBufferSourceNode {
    return new AudioBufferSourceNode(this, this.context.createBufferSource());
  }

  createBuffer(
    numOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer {
    return new AudioBuffer(
      this.context.createBuffer(numOfChannels, length, sampleRate)
    );
  }

  createPeriodicWave(
    real: number[],
    imag: number[],
    constraints?: PeriodicWaveConstraints
  ): PeriodicWave {
    return new PeriodicWave(
      this.context.createPeriodicWave(real, imag, constraints)
    );
  }

  async decodeAudioDataSource(source: string): Promise<AudioBuffer> {
    const arrayBuffer = await fetch(source).then((response) =>
      response.arrayBuffer()
    );

    return new AudioBuffer(await this.context.decodeAudioData(arrayBuffer));
  }

  close(): void {
    this.context.close();
  }
}

export {
  ContextState,
  OscillatorType,
  BiquadFilterType,
  ChannelCountMode,
  ChannelInterpretation,
} from './core/types';

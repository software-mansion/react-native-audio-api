import { ContextState, OfflineAudioContextOptions } from '../types';
import { NotSupportedError } from '../errors';
import { assertSupportedSampleRate } from '../utils/validation';
import BaseAudioContext from './BaseAudioContext.web';
import AnalyserNode from './AnalyserNode.web';
import AudioDestinationNode from './AudioDestinationNode.web';
import AudioListener from './AudioListener.web';
import AudioBuffer from './AudioBuffer.web';
import AudioBufferSourceNode from './AudioBufferSourceNode.web';
import BiquadFilterNode from './BiquadFilterNode.web';
import IIRFilterNode from './IIRFilterNode.web';
import GainNode from './GainNode.web';
import OscillatorNode from './OscillatorNode.web';
import PeriodicWave from './PeriodicWave.web';
import StereoPannerNode from './StereoPannerNode.web';
import ConstantSourceNode from './ConstantSourceNode.web';
import WaveShaperNode from './WaveShaperNode.web';

import ConvolverNode from './ConvolverNode.web';
import DelayNode from './DelayNode.web';
import ChannelMergerNode from './ChannelMergerNode.web';
import ChannelSplitterNode from './ChannelSplitterNode.web';

export default class OfflineAudioContext implements BaseAudioContext {
  readonly context: globalThis.OfflineAudioContext;

  readonly destination: AudioDestinationNode;
  readonly listener: AudioListener;
  readonly sampleRate: number;

  constructor(options: OfflineAudioContextOptions);
  constructor(numberOfChannels: number, length: number, sampleRate: number);
  constructor(
    arg0: OfflineAudioContextOptions | number,
    arg1?: number,
    arg2?: number
  ) {
    if (typeof arg0 === 'object') {
      assertSupportedSampleRate(arg0.sampleRate);
      this.context = new window.OfflineAudioContext(arg0);
    } else if (
      typeof arg0 === 'number' &&
      typeof arg1 === 'number' &&
      typeof arg2 === 'number'
    ) {
      assertSupportedSampleRate(arg2);
      this.context = new window.OfflineAudioContext(arg0, arg1, arg2);
    } else {
      throw new NotSupportedError('Invalid constructor arguments');
    }

    this.sampleRate = this.context.sampleRate;
    this.destination = new AudioDestinationNode(this, this.context.destination);
    this.listener = new AudioListener(this, this.context.listener);
  }

  public get currentTime(): number {
    return this.context.currentTime;
  }

  public get state(): ContextState {
    return this.context.state as ContextState;
  }

  createOscillator(): OscillatorNode {
    return new OscillatorNode(this);
  }

  createConstantSource(): ConstantSourceNode {
    return new ConstantSourceNode(this);
  }

  createGain(): GainNode {
    return new GainNode(this);
  }

  createDelay(maxDelayTime?: number): DelayNode {
    return new DelayNode(this, { maxDelayTime });
  }

  createStereoPanner(): StereoPannerNode {
    return new StereoPannerNode(this);
  }

  createBiquadFilter(): BiquadFilterNode {
    return new BiquadFilterNode(this);
  }

  createConvolver(): ConvolverNode {
    return new ConvolverNode(this);
  }

  createChannelMerger(numberOfInputs?: number): ChannelMergerNode {
    return new ChannelMergerNode(
      this,
      numberOfInputs !== undefined ? { numberOfInputs } : undefined
    );
  }

  createChannelSplitter(numberOfOutputs?: number): ChannelSplitterNode {
    return new ChannelSplitterNode(
      this,
      numberOfOutputs !== undefined ? { numberOfOutputs } : undefined
    );
  }

  createIIRFilter(feedforward: number[], feedback: number[]): IIRFilterNode {
    return new IIRFilterNode(this, { feedforward, feedback });
  }

  createBufferSource(options?: {
    pitchCorrection: boolean;
  }): AudioBufferSourceNode {
    return new AudioBufferSourceNode(this, options);
  }

  createBuffer(
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer {
    if (numberOfChannels < 1 || numberOfChannels > 32) {
      throw new NotSupportedError(
        `The number of channels provided (${numberOfChannels}) is outside the range [1, 32]`
      );
    }

    if (length <= 0) {
      throw new NotSupportedError(
        `The number of frames provided (${length}) is less than or equal to the minimum bound (0)`
      );
    }

    assertSupportedSampleRate(sampleRate);

    return new AudioBuffer({ numberOfChannels, length, sampleRate });
  }

  createPeriodicWave(
    real: Float32Array,
    imag: Float32Array,
    constraints?: PeriodicWaveConstraints
  ): PeriodicWave {
    return new PeriodicWave(this, { real, imag, ...constraints });
  }

  createAnalyser(): AnalyserNode {
    return new AnalyserNode(this);
  }

  createWaveShaper(): WaveShaperNode {
    return new WaveShaperNode(this);
  }

  async decodeAudioDataSource(source: string): Promise<AudioBuffer> {
    const arrayBuffer = await fetch(source).then((response) =>
      response.arrayBuffer()
    );

    return this.decodeAudioData(arrayBuffer);
  }

  async decodeAudioData(arrayBuffer: ArrayBuffer): Promise<AudioBuffer> {
    return new AudioBuffer(await this.context.decodeAudioData(arrayBuffer));
  }

  async startRendering(): Promise<AudioBuffer> {
    return new AudioBuffer(await this.context.startRendering());
  }

  async resume(): Promise<void> {
    await this.context.resume();
  }

  async suspend(suspendTime: number): Promise<void> {
    await this.context.suspend(suspendTime);
  }
}

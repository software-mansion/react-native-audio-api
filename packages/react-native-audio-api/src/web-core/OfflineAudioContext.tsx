import { ContextState, OfflineAudioContextOptions } from '../types';
import { InvalidAccessError, NotSupportedError } from '../errors';
import BaseAudioContext from './BaseAudioContext';
import AnalyserNode from './AnalyserNode';
import AudioDestinationNode from './AudioDestinationNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import BiquadFilterNode from './BiquadFilterNode';
import IIRFilterNode from './IIRFilterNode';
import GainNode from './GainNode';
import OscillatorNode from './OscillatorNode';
import PeriodicWave from './PeriodicWave';
import StereoPannerNode from './StereoPannerNode';
import ConstantSourceNode from './ConstantSourceNode';
import WaveShaperNode from './WaveShaperNode';

import ConvolverNode from './ConvolverNode';
import DelayNode from './DelayNode';

export default class OfflineAudioContext implements BaseAudioContext {
  readonly context: globalThis.OfflineAudioContext;

  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  constructor(options: OfflineAudioContextOptions);
  constructor(numberOfChannels: number, length: number, sampleRate: number);
  constructor(
    arg0: OfflineAudioContextOptions | number,
    arg1?: number,
    arg2?: number
  ) {
    if (typeof arg0 === 'object') {
      this.context = new window.OfflineAudioContext(arg0);
    } else if (
      typeof arg0 === 'number' &&
      typeof arg1 === 'number' &&
      typeof arg2 === 'number'
    ) {
      this.context = new window.OfflineAudioContext(arg0, arg1, arg2);
    } else {
      throw new NotSupportedError('Invalid constructor arguments');
    }

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

  createIIRFilter(feedforward: number[], feedback: number[]): IIRFilterNode {
    return new IIRFilterNode(this, { feedforward, feedback });
  }

  createBufferSource(pitchCorrection?: boolean): AudioBufferSourceNode {
    return new AudioBufferSourceNode(this, { pitchCorrection });
  }

  createBuffer(
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer {
    if (numberOfChannels < 1 || numberOfChannels >= 32) {
      throw new NotSupportedError(
        `The number of channels provided (${numberOfChannels}) is outside the range [1, 32]`
      );
    }

    if (length <= 0) {
      throw new NotSupportedError(
        `The number of frames provided (${length}) is less than or equal to the minimum bound (0)`
      );
    }

    if (sampleRate < 8000 || sampleRate > 96000) {
      throw new NotSupportedError(
        `The sample rate provided (${sampleRate}) is outside the range [8000, 96000]`
      );
    }

    return new AudioBuffer(this, { numberOfChannels, length, sampleRate });
  }

  createPeriodicWave(
    real: Float32Array,
    imag: Float32Array,
    constraints?: PeriodicWaveConstraints
  ): PeriodicWave {
    if (real.length !== imag.length) {
      throw new InvalidAccessError(
        `The lengths of the real (${real.length}) and imaginary (${imag.length}) arrays must match.`
      );
    }

    return new PeriodicWave(this, { real, imag, ...constraints });
  }

  createAnalyser(): AnalyserNode {
    return new AnalyserNode(this);
  }

  createWaveShaper(): WaveShaperNode {
    return new WaveShaperNode(this, this.context.createWaveShaper());
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

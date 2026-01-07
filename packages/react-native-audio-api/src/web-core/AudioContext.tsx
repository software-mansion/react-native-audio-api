import { InvalidAccessError, NotSupportedError } from '../errors';
import {
  AudioBufferBaseSourceNodeOptions,
  AudioContextOptions,
  ContextState,
  DecodeDataInput,
  IIRFilterNodeOptions,
  PeriodicWaveConstraints,
} from '../types';
import AnalyserNode from './AnalyserNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import AudioDestinationNode from './AudioDestinationNode';
import BaseAudioContext from './BaseAudioContext';
import BiquadFilterNode from './BiquadFilterNode';
import ConvolverNode from './ConvolverNode';
import { ConvolverNodeOptions } from './ConvolverNodeOptions';
import DelayNode from './DelayNode';
import GainNode from './GainNode';
import IIRFilterNode from './IIRFilterNode';
import OscillatorNode from './OscillatorNode';
import PeriodicWave from './PeriodicWave';
import StereoPannerNode from './StereoPannerNode';

import ConstantSourceNode from './ConstantSourceNode';
import { globalTag, globalWasmPromise } from './custom/LoadCustomWasm';
import WaveShaperNode from './WaveShaperNode';

export default class AudioContext implements BaseAudioContext {
  readonly context: globalThis.AudioContext;

  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  constructor(options?: AudioContextOptions) {
    if (
      options &&
      options.sampleRate &&
      (options.sampleRate < 8000 || options.sampleRate > 96000)
    ) {
      throw new NotSupportedError(
        `The provided sampleRate is not supported: ${options.sampleRate}`
      );
    }

    this.context = new window.AudioContext({ sampleRate: options?.sampleRate });

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

  createConstantSource(): ConstantSourceNode {
    return new ConstantSourceNode(this, this.context.createConstantSource());
  }

  createGain(): GainNode {
    return new GainNode(this, this.context.createGain());
  }

  createDelay(maxDelayTime?: number): DelayNode {
    return new DelayNode(this, this.context.createDelay(maxDelayTime));
  }

  createStereoPanner(): StereoPannerNode {
    return new StereoPannerNode(this, this.context.createStereoPanner());
  }

  createBiquadFilter(): BiquadFilterNode {
    return new BiquadFilterNode(this, this.context.createBiquadFilter());
  }

  createIIRFilter(options: IIRFilterNodeOptions): IIRFilterNode {
    return new IIRFilterNode(
      this,
      this.context.createIIRFilter(options.feedforward, options.feedback)
    );
  }

  createConvolver(options?: ConvolverNodeOptions): ConvolverNode {
    if (options?.buffer) {
      const numberOfChannels = options.buffer.numberOfChannels;
      if (
        numberOfChannels !== 1 &&
        numberOfChannels !== 2 &&
        numberOfChannels !== 4
      ) {
        throw new NotSupportedError(
          `The number of channels provided (${numberOfChannels}) in impulse response for ConvolverNode buffer must be 1 or 2 or 4.`
        );
      }
    }
    const buffer = options?.buffer ?? null;
    const disableNormalization = options?.disableNormalization ?? false;
    return new ConvolverNode(
      this,
      this.context.createConvolver(),
      buffer,
      disableNormalization
    );
  }

  async createBufferSource(
    options?: AudioBufferBaseSourceNodeOptions
  ): Promise<AudioBufferSourceNode> {
    if (!options || !options.pitchCorrection) {
      return new AudioBufferSourceNode(
        this,
        this.context.createBufferSource(),
        false
      );
    }

    await globalWasmPromise;

    const wasmStretch = await window[globalTag](this.context);

    return new AudioBufferSourceNode(this, wasmStretch, true);
  }

  createBuffer(
    numOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer {
    if (numOfChannels < 1 || numOfChannels >= 32) {
      throw new NotSupportedError(
        `The number of channels provided (${numOfChannels}) is outside the range [1, 32]`
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

    return new AudioBuffer(
      this.context.createBuffer(numOfChannels, length, sampleRate)
    );
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

    return new PeriodicWave(
      this.context.createPeriodicWave(real, imag, constraints)
    );
  }

  createAnalyser(): AnalyserNode {
    return new AnalyserNode(this, this.context.createAnalyser());
  }

  createWaveShaper(): WaveShaperNode {
    return new WaveShaperNode(this, this.context.createWaveShaper());
  }

  async decodeAudioData(
    source: DecodeDataInput,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer> {
    if (source instanceof ArrayBuffer) {
      const decodedData = await this.context.decodeAudioData(source);
      return new AudioBuffer(decodedData);
    }

    if (typeof source === 'string') {
      const response = await fetch(source, fetchOptions);

      if (!response.ok) {
        throw new InvalidAccessError(
          `Failed to fetch audio data from the provided source: ${source}`
        );
      }

      const arrayBuffer = await response.arrayBuffer();
      const decodedData = await this.context.decodeAudioData(arrayBuffer);
      return new AudioBuffer(decodedData);
    }

    throw new TypeError('Unsupported source for decodeAudioData: ' + source);
  }

  async close(): Promise<void> {
    await this.context.close();
  }

  async resume(): Promise<void> {
    await this.context.resume();
  }

  async suspend(): Promise<void> {
    await this.context.suspend();
  }
}

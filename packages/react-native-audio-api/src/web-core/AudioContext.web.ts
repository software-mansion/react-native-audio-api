import { InvalidAccessError, NotSupportedError } from '../errors';
import { AudioContextOptions, ContextState, DecodeDataInput } from '../types';
import AnalyserNode from './AnalyserNode.web';
import AudioBuffer from './AudioBuffer.web';
import AudioBufferSourceNode from './AudioBufferSourceNode.web';
import AudioDestinationNode from './AudioDestinationNode.web';
import BaseAudioContext from './BaseAudioContext.web';
import BiquadFilterNode from './BiquadFilterNode.web';
import ConvolverNode from './ConvolverNode.web';
import DelayNode from './DelayNode.web';
import GainNode from './GainNode.web';
import IIRFilterNode from './IIRFilterNode.web';
import MediaElementAudioSourceNode from './MediaElementAudioSourceNode.web';
import OscillatorNode from './OscillatorNode.web';
import PeriodicWave from './PeriodicWave.web';
import StereoPannerNode from './StereoPannerNode.web';
import ConstantSourceNode from './ConstantSourceNode.web';
import WaveShaperNode from './WaveShaperNode.web';

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

  createBufferSource(options?: {
    pitchCorrection: boolean;
  }): AudioBufferSourceNode {
    return new AudioBufferSourceNode(this, options);
  }

  createMediaElementSource(
    mediaElement: HTMLMediaElement
  ): MediaElementAudioSourceNode {
    return new MediaElementAudioSourceNode(
      this,
      this.context.createMediaElementSource(mediaElement),
      mediaElement
    );
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

    return new AudioBuffer({ numberOfChannels, length, sampleRate });
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

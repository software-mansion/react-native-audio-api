import { InvalidAccessError, NotSupportedError } from '../errors';
import { assertSupportedSampleRate } from '../utils/validation';
import { ContextState, DecodeDataInput } from '../types';
import AnalyserNode from './AnalyserNode.web';
import AudioBuffer from './AudioBuffer.web';
import AudioBufferSourceNode from './AudioBufferSourceNode.web';
import AudioDestinationNode from './AudioDestinationNode.web';
import AudioListener from './AudioListener.web';
import BiquadFilterNode from './BiquadFilterNode.web';
import ChannelMergerNode from './ChannelMergerNode.web';
import ChannelSplitterNode from './ChannelSplitterNode.web';
import ConstantSourceNode from './ConstantSourceNode.web';
import ConvolverNode from './ConvolverNode.web';
import DelayNode from './DelayNode.web';
import GainNode from './GainNode.web';
import IIRFilterNode from './IIRFilterNode.web';
import OscillatorNode from './OscillatorNode.web';
import PeriodicWave from './PeriodicWave.web';
import StereoPannerNode from './StereoPannerNode.web';
import PannerNode from './PannerNode.web';
import WaveShaperNode from './WaveShaperNode.web';

export default abstract class BaseAudioContext<
  BrowserAudioContext extends globalThis.BaseAudioContext =
    globalThis.BaseAudioContext,
> {
  /** @internal */
  readonly context: BrowserAudioContext;

  readonly destination: AudioDestinationNode;
  readonly listener: AudioListener;
  readonly sampleRate: number;

  constructor(context: BrowserAudioContext) {
    this.context = context;
    this.sampleRate = context.sampleRate;
    this.destination = new AudioDestinationNode(this, context.destination);
    this.listener = new AudioListener(this, context.listener);
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

  createPanner(): PannerNode {
    return new PannerNode(this);
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

  async decodeAudioData(
    source: DecodeDataInput,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer> {
    if (source instanceof ArrayBuffer) {
      return new AudioBuffer(await this.context.decodeAudioData(source));
    }

    if (typeof source === 'string') {
      const response = await fetch(source, fetchOptions);

      if (!response.ok) {
        throw new InvalidAccessError(
          `Failed to fetch audio data from the provided source: ${source}`
        );
      }

      const arrayBuffer = await response.arrayBuffer();
      return new AudioBuffer(await this.context.decodeAudioData(arrayBuffer));
    }

    throw new TypeError('Unsupported source for decodeAudioData: ' + source);
  }
}

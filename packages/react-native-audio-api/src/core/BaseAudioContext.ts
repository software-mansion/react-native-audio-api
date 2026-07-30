import { InvalidStateError, NotSupportedError } from '../errors';
import { IBaseAudioContext } from '../jsi-interfaces';
import {
  ContextState,
  DecodeDataInput,
  AudioBufferQueueSourceOptions,
  AudioBufferSourceOptions,
} from '../types';
import AnalyserNode from './AnalyserNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferQueueSourceNode from './AudioBufferQueueSourceNode';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import { decodeAudioData, decodePCMInBase64 } from './AudioDecoder';
import AudioDestinationNode from './AudioDestinationNode';
import AudioListener from './AudioListener';
import BiquadFilterNode from './BiquadFilterNode';
import ConstantSourceNode from './ConstantSourceNode';
import ConvolverNode from './ConvolverNode';
import DelayNode from './DelayNode';
import GainNode from './GainNode';
import IIRFilterNode from './IIRFilterNode';
import OscillatorNode from './OscillatorNode';
import PeriodicWave from './PeriodicWave';
import StereoPannerNode from './StereoPannerNode';
import WaveShaperNode from './WaveShaperNode';

export default class BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly listener: AudioListener;
  readonly sampleRate: number;
  readonly context: IBaseAudioContext;

  constructor(context: IBaseAudioContext) {
    this.context = context;
    this.destination = new AudioDestinationNode(this, context.destination);
    this.listener = new AudioListener(this, context.listener);
    this.sampleRate = context.sampleRate;
  }

  public get currentTime(): number {
    return this.context.currentTime;
  }

  public get state(): ContextState {
    return this.context.state;
  }

  public async decodeAudioData(
    input: DecodeDataInput,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer> {
    return await decodeAudioData(input, this.sampleRate, fetchOptions);
  }

  public async decodePCMInBase64(
    base64String: string,
    inputSampleRate: number,
    inputChannelCount: number,
    isInterleaved: boolean = true
  ): Promise<AudioBuffer> {
    return await decodePCMInBase64(
      base64String,
      inputSampleRate,
      inputChannelCount,
      isInterleaved
    );
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
    if (maxDelayTime !== undefined) {
      return new DelayNode(this, { maxDelayTime });
    } else {
      return new DelayNode(this);
    }
  }

  createStereoPanner(): StereoPannerNode {
    return new StereoPannerNode(this);
  }

  createBiquadFilter(): BiquadFilterNode {
    return new BiquadFilterNode(this);
  }

  createBufferSource(
    options?: AudioBufferSourceOptions
  ): AudioBufferSourceNode {
    if (options !== undefined) {
      return new AudioBufferSourceNode(this, options);
    } else {
      return new AudioBufferSourceNode(this);
    }
  }

  createIIRFilter(feedforward: number[], feedback: number[]): IIRFilterNode {
    if (feedforward.length < 1 || feedforward.length > 20) {
      throw new NotSupportedError(
        `The provided feedforward array has length (${feedforward.length}) outside the range [1, 20]`
      );
    }
    if (feedback.length < 1 || feedback.length > 20) {
      throw new NotSupportedError(
        `The provided feedback array has length (${feedback.length}) outside the range [1, 20]`
      );
    }

    if (feedforward.every((value) => value === 0)) {
      throw new InvalidStateError(
        `Feedforward array must contain at least one non-zero value`
      );
    }

    if (feedback[0] === 0) {
      throw new InvalidStateError(
        `First value of feedback array cannot be zero`
      );
    }

    return new IIRFilterNode(this, { feedforward, feedback });
  }

  createBufferQueueSource(
    options?: AudioBufferQueueSourceOptions
  ): AudioBufferQueueSourceNode {
    if (options !== undefined) {
      return new AudioBufferQueueSourceNode(this, options);
    } else {
      return new AudioBufferQueueSourceNode(this);
    }
  }

  createBuffer(
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer {
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

  createConvolver(): ConvolverNode {
    return new ConvolverNode(this);
  }

  createWaveShaper(): WaveShaperNode {
    return new WaveShaperNode(this);
  }
}

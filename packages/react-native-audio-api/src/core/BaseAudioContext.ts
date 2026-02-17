import {
  InvalidAccessError,
  InvalidStateError,
  NotSupportedError,
} from '../errors';
import { IBaseAudioContext } from '../interfaces';
import { AudioWorkletRuntime, ContextState, DecodeDataInput } from '../types';
import { assertWorkletsEnabled } from '../utils';
import AnalyserNode from './AnalyserNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferQueueSourceNode from './AudioBufferQueueSourceNode';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import { decodeAudioData, decodePCMInBase64 } from './AudioDecoder';
import AudioDestinationNode from './AudioDestinationNode';
import BiquadFilterNode from './BiquadFilterNode';
import ConstantSourceNode from './ConstantSourceNode';
import ConvolverNode from './ConvolverNode';
import DelayNode from './DelayNode';
import GainNode from './GainNode';
import IIRFilterNode from './IIRFilterNode';
import OscillatorNode from './OscillatorNode';
import PeriodicWave from './PeriodicWave';
import RecorderAdapterNode from './RecorderAdapterNode';
import StereoPannerNode from './StereoPannerNode';
import StreamerNode from './StreamerNode';
import WaveShaperNode from './WaveShaperNode';
import WorkletNode from './WorkletNode';
import WorkletProcessingNode from './WorkletProcessingNode';
import WorkletSourceNode from './WorkletSourceNode';

export default class BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;
  readonly context: IBaseAudioContext;

  constructor(context: IBaseAudioContext) {
    this.context = context;
    this.destination = new AudioDestinationNode(this, context.destination);
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

  createWorkletNode(
    callback: (audioData: Array<Float32Array>, channelCount: number) => void,
    bufferLength: number,
    inputChannelCount: number,
    workletRuntime: AudioWorkletRuntime = 'AudioRuntime'
  ): WorkletNode {
    if (inputChannelCount < 1 || inputChannelCount > 32) {
      throw new NotSupportedError(
        `The number of input channels provided (${inputChannelCount}) can not be less than 1 or greater than 32`
      );
    }

    if (bufferLength < 1) {
      throw new NotSupportedError(
        `The buffer length provided (${bufferLength}) can not be less than 1`
      );
    }

    assertWorkletsEnabled();
    return new WorkletNode(
      this,
      workletRuntime,
      callback,
      bufferLength,
      inputChannelCount
    );
  }

  createWorkletProcessingNode(
    callback: (
      inputData: Array<Float32Array>,
      outputData: Array<Float32Array>,
      framesToProcess: number,
      currentTime: number
    ) => void,
    workletRuntime: AudioWorkletRuntime = 'AudioRuntime'
  ): WorkletProcessingNode {
    assertWorkletsEnabled();
    return new WorkletProcessingNode(this, workletRuntime, callback);
  }

  createWorkletSourceNode(
    callback: (
      audioData: Array<Float32Array>,
      framesToProcess: number,
      currentTime: number,
      startOffset: number
    ) => void,
    workletRuntime: AudioWorkletRuntime = 'AudioRuntime'
  ): WorkletSourceNode {
    assertWorkletsEnabled();
    return new WorkletSourceNode(this, workletRuntime, callback);
  }

  createRecorderAdapter(): RecorderAdapterNode {
    return new RecorderAdapterNode(this);
  }

  createOscillator(): OscillatorNode {
    return new OscillatorNode(this);
  }

  createStreamer(): StreamerNode {
    return new StreamerNode(this);
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

  createBufferSource(options?: {
    pitchCorrection: boolean;
  }): AudioBufferSourceNode {
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

  createBufferQueueSource(options?: {
    pitchCorrection: boolean;
  }): AudioBufferQueueSourceNode {
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

  createConvolver(): ConvolverNode {
    return new ConvolverNode(this);
  }

  createWaveShaper(): WaveShaperNode {
    return new WaveShaperNode(this);
  }
}

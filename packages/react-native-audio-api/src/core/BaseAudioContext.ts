import AudioAPIModule from '../AudioAPIModule';
import {
  InvalidAccessError,
  InvalidStateError,
  NotSupportedError,
} from '../errors';
import { IBaseAudioContext } from '../interfaces';
import {
  AudioBufferBaseSourceNodeOptions,
  AudioWorkletRuntime,
  ContextState,
  ConvolverNodeOptions,
  DecodeDataInput,
  IIRFilterNodeOptions,
  PeriodicWaveConstraints,
} from '../types';
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

    const shareableWorklet =
      AudioAPIModule.workletsModule!.makeShareableCloneRecursive(
        (audioBuffers: Array<ArrayBuffer>, channelCount: number) => {
          'worklet';
          const floatAudioData: Array<Float32Array> = audioBuffers.map(
            (buffer) => new Float32Array(buffer)
          );
          callback(floatAudioData, channelCount);
        }
      );

    return new WorkletNode(
      this,
      this.context.createWorkletNode(
        shareableWorklet,
        workletRuntime === 'UIRuntime',
        bufferLength,
        inputChannelCount
      )
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

    const shareableWorklet =
      AudioAPIModule.workletsModule!.makeShareableCloneRecursive(
        (
          inputBuffers: Array<ArrayBuffer>,
          outputBuffers: Array<ArrayBuffer>,
          framesToProcess: number,
          currentTime: number
        ) => {
          'worklet';
          const inputData: Array<Float32Array> = inputBuffers.map(
            (buffer) => new Float32Array(buffer, 0, framesToProcess)
          );
          const outputData: Array<Float32Array> = outputBuffers.map(
            (buffer) => new Float32Array(buffer, 0, framesToProcess)
          );
          callback(inputData, outputData, framesToProcess, currentTime);
        }
      );

    return new WorkletProcessingNode(
      this,
      this.context.createWorkletProcessingNode(
        shareableWorklet,
        workletRuntime === 'UIRuntime'
      )
    );
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
    const shareableWorklet =
      AudioAPIModule.workletsModule!.makeShareableCloneRecursive(
        (
          audioBuffers: Array<ArrayBuffer>,
          framesToProcess: number,
          currentTime: number,
          startOffset: number
        ) => {
          'worklet';
          const floatAudioData: Array<Float32Array> = audioBuffers.map(
            (buffer) => new Float32Array(buffer)
          );
          callback(floatAudioData, framesToProcess, currentTime, startOffset);
        }
      );

    return new WorkletSourceNode(
      this,
      this.context.createWorkletSourceNode(
        shareableWorklet,
        workletRuntime === 'UIRuntime'
      )
    );
  }

  createRecorderAdapter(): RecorderAdapterNode {
    return new RecorderAdapterNode(this, this.context.createRecorderAdapter());
  }

  createOscillator(): OscillatorNode {
    return new OscillatorNode(this, this.context.createOscillator());
  }

  createStreamer(): StreamerNode {
    const streamer = this.context.createStreamer();
    if (!streamer) {
      throw new NotSupportedError('StreamerNode requires FFmpeg build');
    }
    return new StreamerNode(this, streamer);
  }

  createConstantSource(): ConstantSourceNode {
    return new ConstantSourceNode(this, this.context.createConstantSource());
  }

  createGain(): GainNode {
    return new GainNode(this, this.context.createGain());
  }

  createDelay(maxDelayTime?: number): DelayNode {
    const maxTime = maxDelayTime ?? 1.0;
    return new DelayNode(this, this.context.createDelay(maxTime));
  }

  createStereoPanner(): StereoPannerNode {
    return new StereoPannerNode(this, this.context.createStereoPanner());
  }

  createBiquadFilter(): BiquadFilterNode {
    return new BiquadFilterNode(this, this.context.createBiquadFilter());
  }

  createIIRFilter(options: IIRFilterNodeOptions): IIRFilterNode {
    const feedforward = options.feedforward;
    const feedback = options.feedback;
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

    return new IIRFilterNode(
      this,
      this.context.createIIRFilter(feedforward, feedback)
    );
  }

  createBufferSource(
    options?: AudioBufferBaseSourceNodeOptions
  ): AudioBufferSourceNode {
    const pitchCorrection = options?.pitchCorrection ?? false;

    return new AudioBufferSourceNode(
      this,
      this.context.createBufferSource(pitchCorrection)
    );
  }

  createBufferQueueSource(
    options?: AudioBufferBaseSourceNodeOptions
  ): AudioBufferQueueSourceNode {
    const pitchCorrection = options?.pitchCorrection ?? false;

    return new AudioBufferQueueSourceNode(
      this,
      this.context.createBufferQueueSource(pitchCorrection)
    );
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

    const disableNormalization = constraints?.disableNormalization ?? false;

    return new PeriodicWave(
      this.context.createPeriodicWave(real, imag, disableNormalization)
    );
  }

  createAnalyser(): AnalyserNode {
    return new AnalyserNode(this, this.context.createAnalyser());
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
      this.context.createConvolver(buffer?.buffer, disableNormalization)
    );
  }

  createWaveShaper(): WaveShaperNode {
    return new WaveShaperNode(this, this.context.createWaveShaper());
  }
}

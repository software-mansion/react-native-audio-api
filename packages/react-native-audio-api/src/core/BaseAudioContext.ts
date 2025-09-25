import { IBaseAudioContext } from '../interfaces';
import {
  ContextState,
  PeriodicWaveConstraints,
  AudioBufferBaseSourceNodeOptions,
} from '../types';
import AudioDestinationNode from './AudioDestinationNode';
import OscillatorNode from './OscillatorNode';
import GainNode from './GainNode';
import StereoPannerNode from './StereoPannerNode';
import BiquadFilterNode from './BiquadFilterNode';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import AudioBuffer from './AudioBuffer';
import PeriodicWave from './PeriodicWave';
import AnalyserNode from './AnalyserNode';
import AudioBufferQueueSourceNode from './AudioBufferQueueSourceNode';
import StreamerNode from './StreamerNode';
import { InvalidAccessError, NotSupportedError } from '../errors';
import RecorderAdapterNode from './RecorderAdapterNode';
import WorkletNode from './WorkletNode';
import { isWorkletsAvailable, workletsModule } from '../utils';

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

  createWorkletNode(
    callback: (audioData: Array<Float32Array>, channelCount: number) => void,
    bufferLength: number,
    inputChannelCount: number
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

    if (isWorkletsAvailable) {
      const shareableWorklet = workletsModule.makeShareableCloneRecursive(
        (audioBuffers: Array<ArrayBuffer>, channelCount: number) => {
          'worklet';
          const floatAudioData: Array<Float32Array> = audioBuffers.map(
            (buffer) => new Float32Array(buffer)
          );
          callback(floatAudioData, channelCount);

          /// !IMPORTANT Workaround
          /// This is required for now because the worklet is run using runGuarded in C++ which does not invoke any interaction with
          /// the event queue which means if no task is being scheduled, the worklet's side effect won't happen.
          /// So worklet will be called but any of its interactions with the UI thread will not be visible.

          /// This forces to flush queue
          requestAnimationFrame(() => {});
        }
      );
      return new WorkletNode(
        this,
        this.context.createWorkletNode(
          shareableWorklet,
          bufferLength,
          inputChannelCount
        )
      );
    }
    /// User does not have worklets as a dependency so he cannot use the worklet API.
    throw new Error(
      '[RnAudioApi] Worklets are not available, please install react-native-worklets as a dependency. Refer to documentation for more details.'
    );
  }

  createRecorderAdapter(): RecorderAdapterNode {
    return new RecorderAdapterNode(this, this.context.createRecorderAdapter());
  }

  createOscillator(): OscillatorNode {
    return new OscillatorNode(this, this.context.createOscillator());
  }

  createStreamer(): StreamerNode {
    return new StreamerNode(this, this.context.createStreamer());
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

  /** Decodes audio data from a local file path. */
  async decodeAudioDataSource(sourcePath: string): Promise<AudioBuffer> {
    // Remove the file:// prefix if it exists
    if (sourcePath.startsWith('file://')) {
      sourcePath = sourcePath.replace('file://', '');
    }

    return new AudioBuffer(
      await this.context.decodeAudioDataSource(sourcePath)
    );
  }

  /** Decodes audio data from an ArrayBuffer. */
  async decodeAudioData(data: ArrayBuffer): Promise<AudioBuffer> {
    return new AudioBuffer(
      await this.context.decodeAudioData(new Uint8Array(data))
    );
  }

  async decodePCMInBase64Data(
    base64: string,
    playbackRate: number = 1.0
  ): Promise<AudioBuffer> {
    return new AudioBuffer(
      await this.context.decodePCMAudioDataInBase64(base64, playbackRate)
    );
  }
}

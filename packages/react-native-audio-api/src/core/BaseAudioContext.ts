import { IBaseAudioContext } from '../interfaces';
import {
  ContextState,
  PeriodicWaveConstraints,
  AudioBufferSourceNodeOptions,
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
import { InvalidAccessError, NotSupportedError } from '../errors';
import RecorderAdapterNode from './RecorderAdapterNode';

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

  createRecorderAdapter(): RecorderAdapterNode {
    return new RecorderAdapterNode(this, this.context.createRecorderAdapter());
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

  createBufferSource(
    options?: AudioBufferSourceNodeOptions
  ): AudioBufferSourceNode {
    const pitchCorrection = options?.pitchCorrection ?? false;

    return new AudioBufferSourceNode(
      this,
      this.context.createBufferSource(pitchCorrection)
    );
  }

  createBufferQueueSource(): AudioBufferQueueSourceNode {
    return new AudioBufferQueueSourceNode(
      this,
      this.context.createBufferQueueSource()
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

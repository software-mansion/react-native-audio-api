import { IBaseAudioContext } from '../interfaces';
import { ContextState, PeriodicWaveConstraints, AudioSource } from './types';
import AudioDestinationNode from './AudioDestinationNode';
import OscillatorNode from './OscillatorNode';
import GainNode from './GainNode';
import StereoPannerNode from './StereoPannerNode';
import BiquadFilterNode from './BiquadFilterNode';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import AudioBuffer from './AudioBuffer';
import PeriodicWave from './PeriodicWave';
import { InvalidAccessError } from '../errors';
import { resolveAudioSource } from '../utils/resolveAudioSource';

export default class BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;
  protected readonly context: IBaseAudioContext;

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

  createBufferSource(): AudioBufferSourceNode {
    return new AudioBufferSourceNode(this, this.context.createBufferSource());
  }

  createBuffer(
    numOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer {
    if (numOfChannels < 1 || numOfChannels > 3) {
      throw new RangeError(
        `The number of channels provided (${numOfChannels}) is outside the range [1, 2]`
      );
    }

    if (length <= 0) {
      throw new RangeError(
        `The number of frames provided (${length}) is less than or equal to the minimum bound (0)`
      );
    }

    if (sampleRate <= 0) {
      throw new RangeError(
        `The sample rate provided (${sampleRate}) is outside the range [3000, 768000]`
      );
    }

    return new AudioBuffer(
      this.context.createBuffer(numOfChannels, length, sampleRate)
    );
  }

  createPeriodicWave(
    real: number[],
    imag: number[],
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

  decodeAudioDataSource(source: AudioSource | number): AudioBuffer {
    return new AudioBuffer(
      this.context.decodeAudioDataSource(resolveAudioSource(source))
    );
  }
}

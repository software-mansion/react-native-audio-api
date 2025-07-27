import {
  ContextState,
  PeriodicWaveConstraints,
  AudioBufferSourceNodeOptions,
} from '../../types';
import AudioDestinationNode from '../destinations/AudioDestinationNode';
import OscillatorNode from '../sources/OscillatorNode';
import GainNode from '../effects/GainNode';
import StereoPannerNode from '../effects/StereoPannerNode';
import BiquadFilterNode from '../effects/BiquadFilterNode';
import AudioBufferSourceNode from '../sources/AudioBufferSourceNode';
import AudioBuffer from '../sources/AudioBuffer';
import PeriodicWave from '../effects/PeriodicWave';
import AnalyserNode from '../analysis/AnalyserNode';
import { InvalidAccessError, NotSupportedError } from '../../errors';

import { globalWasmPromise, globalTag } from '../custom/LoadCustomWasm';

export default class BaseAudioContext {
  protected readonly context: globalThis.BaseAudioContext;
  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  constructor(context: globalThis.BaseAudioContext) {
    this.context = context;
    this.destination = new AudioDestinationNode(this, this.context.destination);
    this.sampleRate = this.context.sampleRate;
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

  createGain(): GainNode {
    return new GainNode(this, this.context.createGain());
  }

  createStereoPanner(): StereoPannerNode {
    return new StereoPannerNode(this, this.context.createStereoPanner());
  }

  createBiquadFilter(): BiquadFilterNode {
    return new BiquadFilterNode(this, this.context.createBiquadFilter());
  }

  // UNIFIED IT SOMEHOW
  async createBufferSource(
    options?: AudioBufferSourceNodeOptions
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

  async decodeAudioDataSource(source: string): Promise<AudioBuffer> {
    const arrayBuffer = await fetch(source).then((response) =>
      response.arrayBuffer()
    );

    return this.decodeAudioData(arrayBuffer);
  }

  async decodeAudioData(arrayBuffer: ArrayBuffer): Promise<AudioBuffer> {
    return new AudioBuffer(await this.context.decodeAudioData(arrayBuffer));
  }

  // TODO: IMPLEMENT IT
  // eslint-disable-next-line @typescript-eslint/require-await
  async decodePCMInBase64Data(
    base64: string,
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    playbackRate: number = 1.0
  ): Promise<AudioBuffer> {
    console.warn('decodePCMInBase64Data does not work on web');

    return this.createBuffer(2, 48000, this.sampleRate);
  }
}

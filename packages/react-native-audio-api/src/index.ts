import type {
  BaseAudioContext,
  AudioDestinationNode,
  GainNode,
  StereoPannerNode,
  OscillatorNode,
  BiquadFilterNode,
  AudioBufferSourceNode,
  AudioBuffer,
  WaveType,
} from './types';
import { installModule } from './utils/install';

if (global.__AudioAPIInstaller == null) {
  installModule();
}

export class AudioContext implements BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;
  private readonly __AudioContext: BaseAudioContext;

  constructor() {
    this.__AudioContext = global.__AudioAPIInstaller.createAudioContext();

    this.destination = this.__AudioContext.destination;
    this.sampleRate = this.__AudioContext.sampleRate;
  }

  public get currentTime() {
    return this.__AudioContext.currentTime;
  }

  public get state() {
    return this.__AudioContext.state;
  }

  createOscillator(): OscillatorNode {
    return this.__AudioContext.createOscillator();
  }

  createGain(): GainNode {
    return this.__AudioContext.createGain();
  }

  createStereoPanner(): StereoPannerNode {
    return this.__AudioContext.createStereoPanner();
  }

  createBiquadFilter(): BiquadFilterNode {
    return this.__AudioContext.createBiquadFilter();
  }

  createBufferSource(): AudioBufferSourceNode {
    return this.__AudioContext.createBufferSource();
  }

  createBuffer(
    sampleRate: number,
    length: number,
    numOfChannels: number
  ): AudioBuffer {
    return this.__AudioContext.createBuffer(sampleRate, length, numOfChannels);
  }

  close(): void {
    this.__AudioContext.close();
  }
}

export type {
  GainNode,
  StereoPannerNode,
  OscillatorNode,
  BiquadFilterNode,
  AudioBufferSourceNode,
  AudioBuffer,
  WaveType,
};

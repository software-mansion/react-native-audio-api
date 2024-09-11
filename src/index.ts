import type {
  BaseAudioContext,
  AudioDestinationNode,
  GainNode,
  StereoPannerNode,
  OscillatorNode,
  BiquadFilterNode,
  AudioBufferSourceNode,
  AudioBuffer,
} from './types';
import { installModule } from './utils/install';

export class AudioContext implements BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  constructor() {
    if (global.__AudioContext == null) {
      installModule();
    }

    this.destination = global.__AudioContext.destination;
    this.sampleRate = global.__AudioContext.sampleRate;
  }

  public get currentTime() {
    return global.__AudioContext.currentTime;
  }

  public get state() {
    return global.__AudioContext.state;
  }

  createOscillator(): OscillatorNode {
    return global.__AudioContext.createOscillator();
  }

  createGain(): GainNode {
    return global.__AudioContext.createGain();
  }

  createStereoPanner(): StereoPannerNode {
    return global.__AudioContext.createStereoPanner();
  }

  createBiquadFilter(): BiquadFilterNode {
    return global.__AudioContext.createBiquadFilter();
  }

  createBufferSource(): AudioBufferSourceNode {
    return global.__AudioContext.createBufferSource();
  }

  createBuffer(
    sampleRate: number,
    length: number,
    numOfChannels: number
  ): AudioBuffer {
    return global.__AudioContext.createBuffer(
      sampleRate,
      length,
      numOfChannels
    );
  }

  close(): void {
    global.__AudioContext.close();
  }
}

export type { GainNode, StereoPannerNode, OscillatorNode, BiquadFilterNode };

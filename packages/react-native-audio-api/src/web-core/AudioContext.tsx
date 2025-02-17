import { ContextState, PeriodicWaveConstraints } from '../index.native';
import BaseAudioContext from './BaseAudioContext';
import AnalyserNode from './AnalyserNode';
import AudioDestinationNode from './AudioDestinationNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import BiquadFilterNode from './BiquadFilterNode';
import GainNode from './GainNode';
import OscillatorNode from './OscillatorNode';
import PeriodicWave from './PeriodicWave';
import StereoPannerNode from './StereoPannerNode';

export default class AudioContext implements BaseAudioContext {
  readonly context: globalThis.AudioContext;

  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  constructor(_sampleRate?: number) {
    this.context = new window.AudioContext();

    this.sampleRate = this.context.sampleRate;
    this.destination = new AudioDestinationNode(this, this.context.destination);
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

  createBufferSource(): AudioBufferSourceNode {
    return new AudioBufferSourceNode(this, this.context.createBufferSource());
  }

  createBuffer(
    numOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer {
    return new AudioBuffer(
      this.context.createBuffer(numOfChannels, length, sampleRate)
    );
  }

  createPeriodicWave(
    real: number[],
    imag: number[],
    constraints?: PeriodicWaveConstraints
  ): PeriodicWave {
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

    return new AudioBuffer(await this.context.decodeAudioData(arrayBuffer));
  }

  async close(): Promise<void> {
    await this.context.close();
  }
}

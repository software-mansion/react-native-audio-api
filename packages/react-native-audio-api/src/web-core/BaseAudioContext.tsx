import {
  ContextState,
  IIRFilterNodeOptions,
  PeriodicWaveConstraints,
} from '../types';
import AnalyserNode from './AnalyserNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import AudioDestinationNode from './AudioDestinationNode';
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

export default interface BaseAudioContext {
  readonly context: globalThis.BaseAudioContext;

  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  get currentTime(): number;
  get state(): ContextState;
  createOscillator(): OscillatorNode;
  createConstantSource(): ConstantSourceNode;
  createGain(): GainNode;
  createDelay(maxDelayTime?: number): DelayNode;
  createStereoPanner(): StereoPannerNode;
  createBiquadFilter(): BiquadFilterNode;
  createIIRFilter(options: IIRFilterNodeOptions): IIRFilterNode;
  createConvolver(): ConvolverNode;
  createBufferSource(): Promise<AudioBufferSourceNode>;
  createBuffer(
    numOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer;
  createPeriodicWave(
    real: Float32Array,
    imag: Float32Array,
    constraints?: PeriodicWaveConstraints
  ): PeriodicWave;
  createAnalyser(): AnalyserNode;
  createWaveShaper(): WaveShaperNode;
  decodeAudioData(
    arrayBuffer: ArrayBuffer,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer>;
}

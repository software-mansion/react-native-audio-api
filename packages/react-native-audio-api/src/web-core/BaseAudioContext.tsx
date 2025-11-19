import {
  ContextState,
  PeriodicWaveConstraints,
  IIRFilterNodeOptions,
} from '../types';
import AnalyserNode from './AnalyserNode';
import AudioDestinationNode from './AudioDestinationNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import BiquadFilterNode from './BiquadFilterNode';
import IIRFilterNode from './IIRFilterNode';
import GainNode from './GainNode';
import OscillatorNode from './OscillatorNode';
import PeriodicWave from './PeriodicWave';
import StereoPannerNode from './StereoPannerNode';
import ConstantSourceNode from './ConstantSourceNode';
import ConvolverNode from './ConvolverNode';

export default interface BaseAudioContext {
  readonly context: globalThis.BaseAudioContext;

  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  get currentTime(): number;
  get state(): ContextState;
  createOscillator(): OscillatorNode;
  createConstantSource(): ConstantSourceNode;
  createGain(): GainNode;
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
  decodeAudioDataSource(source: string): Promise<AudioBuffer>;
  decodeAudioData(arrayBuffer: ArrayBuffer): Promise<AudioBuffer>;
}

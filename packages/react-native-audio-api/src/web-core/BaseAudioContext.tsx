import { ContextState, PeriodicWaveConstraints } from '../index.native';
import AnalyserNode from './AnalyserNode';
import AudioDestinationNode from './AudioDestinationNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import BiquadFilterNode from './BiquadFilterNode';
import GainNode from './GainNode';
import OscillatorNode from './OscillatorNode';
import PeriodicWave from './PeriodicWave';
import StereoPannerNode from './StereoPannerNode';

export default interface BaseAudioContext {
  readonly context: globalThis.BaseAudioContext;

  readonly destination: AudioDestinationNode;
  readonly sampleRate: number;

  get currentTime(): number;
  get state(): ContextState;
  createOscillator(): OscillatorNode;
  createGain(): GainNode;
  createStereoPanner(): StereoPannerNode;
  createBiquadFilter(): BiquadFilterNode;
  createBufferSource(): AudioBufferSourceNode;
  createBuffer(
    numOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer;
  createPeriodicWave(
    real: number[],
    imag: number[],
    constraints?: PeriodicWaveConstraints
  ): PeriodicWave;
  createAnalyser(): AnalyserNode;
  decodeAudioDataSource(source: string): Promise<AudioBuffer>;
}

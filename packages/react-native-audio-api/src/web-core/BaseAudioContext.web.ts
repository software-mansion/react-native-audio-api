import { ContextState } from '../types';
import AnalyserNode from './AnalyserNode.web';
import AudioBuffer from './AudioBuffer.web';
import AudioBufferSourceNode from './AudioBufferSourceNode.web';
import AudioDestinationNode from './AudioDestinationNode.web';
import BiquadFilterNode from './BiquadFilterNode.web';
import ConstantSourceNode from './ConstantSourceNode.web';
import ConvolverNode from './ConvolverNode.web';
import DelayNode from './DelayNode.web';
import GainNode from './GainNode.web';
import IIRFilterNode from './IIRFilterNode.web';
import OscillatorNode from './OscillatorNode.web';
import PeriodicWave from './PeriodicWave.web';
import StereoPannerNode from './StereoPannerNode.web';
import WaveShaperNode from './WaveShaperNode.web';

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
  createIIRFilter(feedforward: number[], feedback: number[]): IIRFilterNode;
  createConvolver(): ConvolverNode;
  createBufferSource(options?: {
    pitchCorrection: boolean;
  }): AudioBufferSourceNode;
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

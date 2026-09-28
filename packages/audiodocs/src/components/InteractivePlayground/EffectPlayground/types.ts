import type { AudioContext, AudioNode } from 'react-native-audio-api';

export type FrequencyResponseReader = (
  frequencies: Float32Array<ArrayBuffer>,
  magnitudes: Float32Array<ArrayBuffer>,
  phases: Float32Array<ArrayBuffer>
) => void;

/**
 * One live effect instance inside the playground graph.
 *
 * The chain wires the source into `input` and `output` into the destination once and never
 * rewires them, so an implementation whose inner node cannot be mutated (IIR coefficients,
 * wave shaper curves) has to swap that node behind stable pass-through nodes.
 */
export interface EffectHandle<TParams> {
  readonly input: AudioNode;
  readonly output: AudioNode;
  update(params: TParams): void;
  /** Magnitude response of the current node, drawn over the live spectrum when provided. */
  getFrequencyResponse?: FrequencyResponseReader;
  dispose(): void;
}

export type EffectFactory<TParams> = (ctx: AudioContext, params: TParams) => EffectHandle<TParams>;

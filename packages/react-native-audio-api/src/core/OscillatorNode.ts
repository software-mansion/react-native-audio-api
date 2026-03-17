import { IOscillatorNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';
import PeriodicWave from './PeriodicWave';
import { InvalidStateError } from '../errors';
import { OscillatorOptions } from '../types';

export default class OscillatorNode extends AudioScheduledSourceNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;

  constructor(context: BaseAudioContext, options?: OscillatorOptions) {
    if (options?.periodicWave) {
      options.type = 'custom';
    }

    const node = context.context.createOscillator(options || {});
    super(context, node);
    this.frequency = new AudioParam(node.frequency, context);
    this.detune = new AudioParam(node.detune, context);
    this.type = node.type;
  }

  public get type(): OscillatorType {
    return (this.node as IOscillatorNode).type;
  }

  public set type(value: OscillatorType) {
    if (value === 'custom') {
      throw new InvalidStateError(
        "'type' cannot be set directly to 'custom'.  Use setPeriodicWave() to create a custom Oscillator type."
      );
    }

    (this.node as IOscillatorNode).type = value;
  }

  public setPeriodicWave(wave: PeriodicWave): void {
    (this.node as IOscillatorNode).setPeriodicWave(wave.periodicWave);
  }
}

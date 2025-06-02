import { OscillatorType } from '../types';
import { InvalidStateError } from '../errors';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';
import AudioParam from './AudioParam';
import PeriodicWave from './PeriodicWave';

export default class OscillatorNode extends AudioScheduledSourceNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;

  constructor(context: BaseAudioContext, node: globalThis.OscillatorNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune, context);
    this.frequency = new AudioParam(node.frequency, context);
  }

  public get type(): OscillatorType {
    return (this.node as globalThis.OscillatorNode).type;
  }

  public set type(value: OscillatorType) {
    if (value === 'custom') {
      throw new InvalidStateError(
        "'type' cannot be set directly to 'custom'.  Use setPeriodicWave() to create a custom Oscillator type."
      );
    }

    (this.node as globalThis.OscillatorNode).type = value;
  }

  public setPeriodicWave(wave: PeriodicWave): void {
    (this.node as globalThis.OscillatorNode).setPeriodicWave(wave.periodicWave);
  }
}

import { IOscillatorNode } from '../interfaces';
import { OscillatorType } from './types';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';
import PeriodicWave from './PeriodicWave';
import { InvalidStateError } from '../errors';

export default class OscillatorNode extends AudioScheduledSourceNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;

  constructor(context: BaseAudioContext, node: IOscillatorNode) {
    super(context, node);
    this.frequency = new AudioParam(node.frequency);
    this.detune = new AudioParam(node.detune);
    this.type = node.type;
  }

  public get type(): OscillatorType {
    return (this.node as IOscillatorNode).type;
  }

  public set type(value: OscillatorType) {
    if (value === 'custom') {
      throw new InvalidStateError(
        "The type can't be set to custom. You need to call setPeriodicWave() instead in order to define a custom waveform."
      );
    }

    (this.node as IOscillatorNode).type = value;
  }

  public setPeriodicWave(wave: PeriodicWave): void {
    (this.node as IOscillatorNode).setPeriodicWave(wave.periodicWave);
  }
}

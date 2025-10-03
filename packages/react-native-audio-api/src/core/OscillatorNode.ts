import { IOscillatorNode } from '../interfaces';
import { OscillatorType } from '../types';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';
import PeriodicWave from './PeriodicWave';
import { InvalidStateError } from '../errors';
import { EventEmptyType } from '../events/types';

export default class OscillatorNode extends AudioScheduledSourceNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;

  constructor(context: BaseAudioContext, node: IOscillatorNode) {
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

  public override get onEnded(): ((event: EventEmptyType) => void) | undefined {
    return super.onEnded as ((event: EventEmptyType) => void) | undefined;
  }

  public override set onEnded(
    callback: ((event: EventEmptyType) => void) | null
  ) {
    super.onEnded = callback;
  }
}

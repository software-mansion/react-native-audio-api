import { IAudioScheduledSourceNode } from '../interfaces';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';

export default class AudioScheduledSourceNode extends AudioNode {
  constructor(context: BaseAudioContext, node: IAudioScheduledSourceNode) {
    super(context, node);
  }

  public start(time: number): void {
    (this.node as IAudioScheduledSourceNode).start(time);
  }

  public stop(time: number): void {
    (this.node as IAudioScheduledSourceNode).stop(time);
  }
}

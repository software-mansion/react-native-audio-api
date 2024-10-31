import { IAudioScheduledSourceNode } from '../interfaces';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';

export default class AudioScheduledSourceNode extends AudioNode {
  constructor(context: BaseAudioContext, node: IAudioScheduledSourceNode) {
    super(context, node);
  }

  public start(when: number = 0): void {
    (this.node as IAudioScheduledSourceNode).start(when);
  }

  public stop(when: number = 0): void {
    (this.node as IAudioScheduledSourceNode).stop(when);
  }
}

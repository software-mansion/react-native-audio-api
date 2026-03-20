import { IAudioFileSourceNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';

export default class AudioFileSourceNode extends AudioScheduledSourceNode {
  constructor(context: BaseAudioContext, source: ArrayBuffer | string) {
    const node = context.context.createFileSource(source);
    super(context, node);
  }

  get volume(): number {
    return (this.node as IAudioFileSourceNode).volume ?? 1;
  }

  set volume(value: number) {
    (this.node as IAudioFileSourceNode).volume = value;
  }
}

import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';

export default class AudioFileSourceNode extends AudioScheduledSourceNode {
  constructor(context: BaseAudioContext, source: ArrayBuffer | string) {
    const node = context.context.createFileSource(source);
    super(context, node);
  }
}

import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';

export default class AudioFileSourceNode extends AudioScheduledSourceNode {
  constructor(context: BaseAudioContext, arrayBuffer: ArrayBuffer) {
    const node = context.context.createFileSource(arrayBuffer);
    super(context, node);
  }
}

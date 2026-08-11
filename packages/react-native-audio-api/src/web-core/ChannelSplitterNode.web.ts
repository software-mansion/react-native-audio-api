import BaseAudioContext from './BaseAudioContext.web';
import AudioNode from './AudioNode.web';
import { ChannelSplitterOptions } from '../types';

export default class ChannelSplitterNode extends AudioNode {
  constructor(context: BaseAudioContext, options?: ChannelSplitterOptions) {
    const node = new globalThis.ChannelSplitterNode(context.context, options);
    super(context, node);
  }
}

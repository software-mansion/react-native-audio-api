import BaseAudioContext from './BaseAudioContext.web';
import AudioNode from './AudioNode.web';
import { ChannelMergerOptions } from '../types';

export default class ChannelMergerNode extends AudioNode {
  constructor(context: BaseAudioContext, options?: ChannelMergerOptions) {
    const node = new globalThis.ChannelMergerNode(context.context, options);
    super(context, node);
  }
}

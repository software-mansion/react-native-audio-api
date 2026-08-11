import { IChannelSplitterNode } from '../jsi-interfaces';
import { ChannelSplitterOptions } from '../types';
import AudioNode from './AudioNode';
import type BaseAudioContext from './BaseAudioContext';
import { validateChannelSplitterOptions } from '../utils/validation/channelMergerSplitter';

export default class ChannelSplitterNode extends AudioNode {
  constructor(context: BaseAudioContext, options?: ChannelSplitterOptions) {
    validateChannelSplitterOptions(options);
    const node: IChannelSplitterNode = context.context.createChannelSplitter(
      options || {}
    );
    super(context, node);
  }
}

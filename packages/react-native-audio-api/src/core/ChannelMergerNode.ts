import { IChannelMergerNode } from '../jsi-interfaces';
import { ChannelMergerOptions } from '../types';
import AudioNode from './AudioNode';
import type BaseAudioContext from './BaseAudioContext';
import { validateChannelMergerOptions } from '../utils/validation/channelMergerSplitter';

export default class ChannelMergerNode extends AudioNode {
  constructor(context: BaseAudioContext, options?: ChannelMergerOptions) {
    validateChannelMergerOptions(options);
    const node: IChannelMergerNode = context.context.createChannelMerger(
      options || {}
    );
    super(context, node);
  }
}

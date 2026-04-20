import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import { StreamerOptions } from '../types';
import { NotSupportedError } from '../errors';
import type BaseAudioContext from './BaseAudioContext';

export default class StreamerNode extends AudioScheduledSourceNode {
  readonly streamPath: string;

  constructor(context: BaseAudioContext, options: StreamerOptions) {
    const node = context.context.createStreamer(options);
    if (!node) {
      throw new NotSupportedError('StreamerNode requires FFmpeg build');
    }
    super(context, node);
    this.streamPath = options.streamPath;
  }
}

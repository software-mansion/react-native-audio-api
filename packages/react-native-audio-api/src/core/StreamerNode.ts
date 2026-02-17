import { IStreamerNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import { StreamerOptions } from '../types';
import { InvalidStateError, NotSupportedError } from '../errors';
import BaseAudioContext from './BaseAudioContext';

export default class StreamerNode extends AudioScheduledSourceNode {
  private hasBeenSetup: boolean = false;
  constructor(context: BaseAudioContext, options?: StreamerOptions) {
    const node = context.context.createStreamer(options || {});
    if (!node) {
      throw new NotSupportedError('StreamerNode requires FFmpeg build');
    }
    super(context, node);
    if (options?.streamPath) {
      if (this.initialize(options.streamPath)) {
        this.hasBeenSetup = true;
      }
    }
  }

  public initialize(streamPath: string): boolean {
    if (this.hasBeenSetup) {
      throw new InvalidStateError('Node is already setup');
    }
    const res = (this.node as IStreamerNode).initialize(streamPath);
    if (res) {
      this.hasBeenSetup = true;
    }
    return res;
  }

  public get streamPath(): string {
    return (this.node as IStreamerNode).streamPath;
  }
}

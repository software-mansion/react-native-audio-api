import { IStreamerNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';

export default class StreamerNode extends AudioScheduledSourceNode {
  public initialize(streamPath: string): boolean {
    return (this.node as IStreamerNode).initialize(streamPath);
  }

  public startStreaming(): void {
    (this.node as IStreamerNode).startStreaming();
  }

  public stopStreaming(): void {
    (this.node as IStreamerNode).stopStreaming();
  }
}

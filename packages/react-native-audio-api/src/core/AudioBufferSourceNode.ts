import { IAudioBufferSourceNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';
import AudioBuffer from './AudioBuffer';

export default class AudioBufferSourceNode extends AudioScheduledSourceNode {
  constructor(context: BaseAudioContext, node: IAudioBufferSourceNode) {
    super(context, node);
  }

  public get buffer(): AudioBuffer {
    return new AudioBuffer((this.node as IAudioBufferSourceNode).buffer);
  }

  public set buffer(buffer: AudioBuffer) {
    (this.node as IAudioBufferSourceNode).buffer = buffer.buffer;
  }

  public get loop(): boolean {
    return (this.node as IAudioBufferSourceNode).loop;
  }

  public set loop(value: boolean) {
    (this.node as IAudioBufferSourceNode).loop = value;
  }
}

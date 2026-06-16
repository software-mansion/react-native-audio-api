import AudioParam from './AudioParam.web';
import AudioBuffer from './AudioBuffer.web';
import BaseAudioContext from './BaseAudioContext.web';
import AudioNode from './AudioNode.web';

import { AudioBufferSourceOptions } from '../types';
import { AudioBufferSourceNodeStretcher } from './custom';
import { AudioBufferSourceNodeStandard } from './utils';

export default class AudioBufferSourceNode {
  private node: AudioBufferSourceNodeStretcher | AudioBufferSourceNodeStandard;

  constructor(context: BaseAudioContext, options?: AudioBufferSourceOptions) {
    this.node = options?.pitchCorrection
      ? new AudioBufferSourceNodeStretcher(context, options)
      : new AudioBufferSourceNodeStandard(context, options);
  }

  connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    return this.node.connect(destination);
  }

  disconnect(destination?: AudioNode | AudioParam): void {
    this.node.disconnect(destination);
  }

  start(when: number = 0, offset?: number, duration?: number): void {
    this.node.start(when, offset, duration);
  }

  stop(when: number = 0): void {
    this.node.stop(when);
  }

  get detune(): AudioParam {
    return this.node.detune;
  }

  get playbackRate(): AudioParam {
    return this.node.playbackRate;
  }

  get buffer(): AudioBuffer | null {
    return this.node.buffer;
  }

  set buffer(buffer: AudioBuffer | null) {
    this.node.buffer = buffer;
  }

  get loop(): boolean {
    return this.node.loop;
  }

  set loop(value: boolean) {
    this.node.loop = value;
  }

  get loopStart(): number {
    return this.node.loopStart;
  }

  set loopStart(value: number) {
    this.node.loopStart = value;
  }

  get loopEnd(): number {
    return this.node.loopEnd;
  }

  set loopEnd(value: number) {
    this.node.loopEnd = value;
  }

  get loopSkip(): boolean {
    return this.node.loopSkip;
  }

  set loopSkip(value: boolean) {
    this.node.loopSkip = value;
  }

  get onLoopEnded(): ((event: object) => void) | undefined {
    return this.node.onLoopEnded;
  }

  set onLoopEnded(callback: ((event: object) => void) | null) {
    this.node.onLoopEnded = callback;
  }
}

import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';
import { AudioWorkletRuntime } from '../types';
import AudioAPIModule from '../AudioAPIModule';

export default class WorkletNode extends AudioNode {
  constructor(
    context: BaseAudioContext,
    runtime: AudioWorkletRuntime,
    callback: (audioData: Array<Float32Array>, channelCount: number) => void,
    bufferLength: number,
    inputChannelCount: number
  ) {
    const shareableWorklet =
      AudioAPIModule.workletsModule!.makeShareableCloneRecursive(
        (audioBuffers: Array<ArrayBuffer>, channelCount: number) => {
          'worklet';
          const floatAudioData: Array<Float32Array> = audioBuffers.map(
            (buffer) => new Float32Array(buffer)
          );
          callback(floatAudioData, channelCount);
        }
      );
    const node = context.context.createWorkletNode(
      shareableWorklet,
      runtime === 'UIRuntime',
      bufferLength,
      inputChannelCount
    );
    super(context, node);
  }
}

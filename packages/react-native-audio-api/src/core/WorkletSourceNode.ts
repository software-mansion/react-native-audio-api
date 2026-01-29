import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';
import { AudioWorkletRuntime } from '../types';
import AudioAPIModule from '../AudioAPIModule';

export default class WorkletSourceNode extends AudioScheduledSourceNode {
  constructor(
    context: BaseAudioContext,
    runtime: AudioWorkletRuntime,
    callback: (
      audioData: Array<Float32Array>,
      framesToProcess: number,
      currentTime: number,
      startOffset: number
    ) => void
  ) {
    const shareableWorklet =
      AudioAPIModule.workletsModule!.makeShareableCloneRecursive(
        (
          audioBuffers: Array<ArrayBuffer>,
          framesToProcess: number,
          currentTime: number,
          startOffset: number
        ) => {
          'worklet';
          const floatAudioData: Array<Float32Array> = audioBuffers.map(
            (buffer) => new Float32Array(buffer)
          );
          callback(floatAudioData, framesToProcess, currentTime, startOffset);
        }
      );
    const node = context.context.createWorkletSourceNode(
      shareableWorklet,
      runtime === 'UIRuntime'
    );
    super(context, node);
  }
}

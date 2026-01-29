import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';
import { AudioWorkletRuntime } from '../types';
import AudioAPIModule from '../AudioAPIModule';

export default class WorkletProcessingNode extends AudioNode {
  constructor(
    context: BaseAudioContext,
    runtime: AudioWorkletRuntime,
    callback: (
      inputData: Array<Float32Array>,
      outputData: Array<Float32Array>,
      framesToProcess: number,
      currentTime: number
    ) => void
  ) {
    const shareableWorklet =
      AudioAPIModule.workletsModule!.makeShareableCloneRecursive(
        (
          inputBuffers: Array<ArrayBuffer>,
          outputBuffers: Array<ArrayBuffer>,
          framesToProcess: number,
          currentTime: number
        ) => {
          'worklet';
          const inputData: Array<Float32Array> = inputBuffers.map(
            (buffer) => new Float32Array(buffer, 0, framesToProcess)
          );
          const outputData: Array<Float32Array> = outputBuffers.map(
            (buffer) => new Float32Array(buffer, 0, framesToProcess)
          );
          callback(inputData, outputData, framesToProcess, currentTime);
        }
      );
    const node = context.context.createWorkletProcessingNode(
      shareableWorklet,
      runtime === 'UIRuntime'
    );
    super(context, node);
  }
}

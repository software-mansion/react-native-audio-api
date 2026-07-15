import {
  AudioNode,
  BaseAudioContext,
  NotSupportedError,
} from 'react-native-audio-api';

import AudioWorkletsModule from './AudioWorkletsModule';
import type { WorkletNodeCallback, WorkletNodeOptions } from './types';
import {
  resolveWorkletNodeOptions,
  validateWorkletSmoothingTimeConstant,
  workletNodeDomainToNative,
} from './utils';

export type { WorkletNodeDomain, WorkletNodeOptions } from './types';

interface IWorkletNode {
  readonly bufferLength: number;
  smoothingTimeConstant: number;
}

export default class WorkletNode extends AudioNode {
  constructor(
    context: BaseAudioContext,
    callback: WorkletNodeCallback,
    options?: number | WorkletNodeOptions
  ) {
    const resolved = resolveWorkletNodeOptions(options);

    const workletsModule = AudioWorkletsModule.workletsModule;
    const shareableWorklet = workletsModule.createSerializable(callback);

    if (globalThis.__createWorkletNode == null) {
      throw new NotSupportedError(
        'react-native-audio-worklets: worklet extensions are not installed.'
      );
    }

    const node = globalThis.__createWorkletNode(
      context.context,
      shareableWorklet,
      workletNodeDomainToNative(resolved.domain),
      resolved.bufferLength,
      resolved.smoothingTimeConstant,
      workletsModule.getUIRuntimeHolder(),
      workletsModule.getUISchedulerHolder()
    );

    super(context, node);
  }

  public get bufferLength(): number {
    return (this.node as unknown as IWorkletNode).bufferLength;
  }

  public get smoothingTimeConstant(): number {
    return (this.node as unknown as IWorkletNode).smoothingTimeConstant;
  }

  public set smoothingTimeConstant(value: number) {
    validateWorkletSmoothingTimeConstant(value);
    (this.node as unknown as IWorkletNode).smoothingTimeConstant = value;
  }
}

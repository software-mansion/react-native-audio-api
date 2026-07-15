import {
  AudioNode,
  BaseAudioContext,
  NotSupportedError,
} from 'react-native-audio-api';

import AudioWorkletsModule from './AudioWorkletsModule';
import type { WorkletNodeCallback } from './types';
import { validateBufferLength } from './utils';

export default class WorkletNode extends AudioNode {
  constructor(
    context: BaseAudioContext,
    callback: WorkletNodeCallback,
    bufferLength: number
  ) {
    const length = validateBufferLength(bufferLength);

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
      length,
      workletsModule.getUIRuntimeHolder(),
      workletsModule.getUISchedulerHolder()
    );

    super(context, node);
  }
}

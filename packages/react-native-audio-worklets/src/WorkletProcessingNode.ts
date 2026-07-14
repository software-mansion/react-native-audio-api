import {
  AudioNode,
  BaseAudioContext,
  NotSupportedError,
} from 'react-native-audio-api';

import AudioWorkletsModule from './AudioWorkletsModule';
import type { WorkletProcessingNodeCallback } from './types';

export default class WorkletProcessingNode extends AudioNode {
  constructor(
    context: BaseAudioContext,
    callback: WorkletProcessingNodeCallback
  ) {
    if (globalThis.__createWorkletProcessingNode == null) {
      throw new NotSupportedError(
        'react-native-audio-worklets: worklet extensions are not installed.'
      );
    }

    const workletsModule = AudioWorkletsModule.workletsModule;
    const shareableWorklet = workletsModule.createSerializable(callback);

    const node = globalThis.__createWorkletProcessingNode(
      context.context,
      shareableWorklet,
      AudioWorkletsModule.getAudioRuntime()
    );

    super(context, node);
  }
}

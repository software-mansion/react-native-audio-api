import {
  AudioScheduledSourceNode,
  BaseAudioContext,
  NotSupportedError,
} from 'react-native-audio-api';

import AudioWorkletsModule from './AudioWorkletsModule';
import type { WorkletSourceNodeCallback } from './types';

export default class WorkletSourceNode extends AudioScheduledSourceNode {
  constructor(context: BaseAudioContext, callback: WorkletSourceNodeCallback) {
    if (globalThis.__createWorkletSourceNode == null) {
      throw new NotSupportedError(
        'react-native-audio-worklets: worklet extensions are not installed.'
      );
    }

    const workletsModule = AudioWorkletsModule.workletsModule;
    const shareableWorklet = workletsModule.createSerializable(callback);

    const node = globalThis.__createWorkletSourceNode(
      context.context,
      shareableWorklet,
      AudioWorkletsModule.getAudioRuntime()
    );

    super(context, node);
  }
}

import { AudioNode, BaseAudioContext } from 'react-native-audio-api';

import AudioWorkletsModule from './AudioWorkletsModule';
import type { WorkletNodeCallback } from './types';

export default class WorkletNode extends AudioNode {
  constructor(context: BaseAudioContext, callback: WorkletNodeCallback) {
    const workletsModule = AudioWorkletsModule.workletsModule;
    const shareableWorklet = workletsModule.createSerializable(callback);

    if (globalThis.__createWorkletNode == null) {
      throw new Error(
        'react-native-audio-worklets: worklet extensions are not installed.'
      );
    }

    const node = globalThis.__createWorkletNode(
      context.context,
      shareableWorklet,
      workletsModule.getUIRuntimeHolder(),
      workletsModule.getUISchedulerHolder()
    );

    super(context, node);
  }
}

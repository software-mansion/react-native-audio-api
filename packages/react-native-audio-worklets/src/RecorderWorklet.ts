import { AudioConsumer, NotSupportedError } from 'react-native-audio-api';

import AudioWorkletsModule from './AudioWorkletsModule';
import type { WorkletNodeCallback, WorkletNodeOptions } from './types';
import {
  resolveWorkletNodeOptions,
  validateWorkletSmoothingTimeConstant,
} from './utils';

interface IRecorderWorklet {
  readonly bufferLength: number;
  smoothingTimeConstant: number;
}

/**
 * Hands snapshots of what an `AudioRecorder` captures to a worklet on the UI
 * runtime, with no `AudioContext`. Add it with
 * `recorder.addConsumer(worklet)`.
 *
 * Snapshots work as in {@link WorkletNode}, from the recorder's input mixed down
 * to mono at the input's sample rate, so frequency-domain bins follow the
 * input's sample rate, not a context's.
 */
export default class RecorderWorklet extends AudioConsumer {
  private readonly native: IRecorderWorklet;

  constructor(
    callback: WorkletNodeCallback,
    options?: number | WorkletNodeOptions
  ) {
    const resolved = resolveWorkletNodeOptions(options);

    const workletsModule = AudioWorkletsModule.workletsModule;
    const shareableWorklet = workletsModule.createSerializable(callback);

    if (globalThis.__createRecorderWorklet == null) {
      throw new NotSupportedError(
        'react-native-audio-worklets: worklet extensions are not installed.'
      );
    }

    const native = globalThis.__createRecorderWorklet(
      shareableWorklet,
      resolved.domain,
      resolved.bufferLength,
      resolved.smoothingTimeConstant,
      workletsModule.getUIRuntimeHolder(),
      workletsModule.getUISchedulerHolder()
    );
    super(native);
    this.native = native;
  }

  public get bufferLength(): number {
    return this.native.bufferLength;
  }

  public get smoothingTimeConstant(): number {
    return this.native.smoothingTimeConstant;
  }

  public set smoothingTimeConstant(value: number) {
    validateWorkletSmoothingTimeConstant(value);
    this.native.smoothingTimeConstant = value;
  }
}

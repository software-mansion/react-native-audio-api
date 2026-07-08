export interface IWorkletsModule {
  /** Creates a serializable value. */
  createSerializable: <T>(value: T) => T;
  /** Returns the holder object wrapping the shared UI worklet runtime. */
  getUIRuntimeHolder: () => object;
  /** Returns the holder object wrapping the shared UI scheduler. */
  getUISchedulerHolder: () => object;
}

/**
 * Invoked on the UI worklet runtime once `bufferLength` frames have been
 * accumulated (when the prior callback has finished).
 *
 * @param audioBuffers - One `ArrayBuffer` per channel of **32-bit float PCM**
 *   (not interleaved), each of length `bufferLength`. Wrap each buffer in a
 *   Float32Array view for zero-copy access.
 * @param numberOfChannels - Active channel count for this callback (same as
 *   `audioBuffers.length`). Use when iterating channels in a loop.
 *
 *   The function must include the `'worklet'` directive.
 */
export type WorkletNodeCallback = (
  audioBuffers: Array<ArrayBuffer>,
  numberOfChannels: number
) => void;

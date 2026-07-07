export interface IWorkletsModule {
  /** Creates a serializable value. */
  createSerializable: <T>(value: T) => T;
  /** Returns the holder object wrapping the shared UI worklet runtime. */
  getUIRuntimeHolder: () => object;
  /** Returns the holder object wrapping the shared UI scheduler. */
  getUISchedulerHolder: () => object;
}

/**
 * Invoked on the UI worklet runtime at most ~120 times per second with the
 * latest render-quantum snapshot (when the prior callback has finished).
 *
 * @param audioBuffers - One `ArrayBuffer` per channel of **32-bit float PCM**
 *   (not interleaved). Wrap with `new Float32Array(buffer)` for zero-copy
 *   access.
 * @param numberOfChannels - Active channel count for this quantum (same as
 *   `audioBuffers.length`). Use when iterating channels in a loop.
 *
 *   The function must include the `'worklet'` directive.
 */
export type WorkletNodeCallback = (
  audioBuffers: Array<ArrayBuffer>,
  numberOfChannels: number
) => void;

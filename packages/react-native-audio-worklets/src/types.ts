export type WorkletNodeDomain = 'time-domain' | 'frequency-domain';

export interface WorkletNodeOptions {
  domain?: WorkletNodeDomain;
  bufferLength?: number;
  smoothingTimeConstant?: number;
}

/**
 * Invoked on the UI worklet runtime once a snapshot is ready (when the prior
 * callback has finished).
 *
 * @remarks
 *   Do not store `audioData` in Reanimated shared values or other state read
 *   asynchronously later. The backing memory is reused for the next snapshot
 *   and may be written from the audio thread after your callback returns.
 *   Derive scalars inside the callback (e.g. RMS, peak) or copy samples
 *   (`Float32Array.from(audioData)`) if you need to keep waveform data for
 *   later UI reads.
 * @param audioData - Stable `Float32Array` view (zero-copy over a reused native
 *   buffer pool). The same object identity is passed on every callback; only
 *   the underlying sample memory is refilled.
 *
 *   - **Time-domain:** down-mixed mono PCM of length `bufferLength`.
 *   - **Frequency-domain:** linear magnitude spectrum of length `bufferLength`
 *       (internal FFT size is `bufferLength * 2`; same window + FFT path as
 *       `AnalyserNode`, with `smoothingTimeConstant` applied at runtime).
 */
export type WorkletNodeCallback = (audioData: Float32Array) => void;

/**
 * Invoked synchronously on the audio worklet runtime each render quantum.
 *
 * @param audioData - Stable per-channel `Float32Array` views over the output
 *   pool. Write samples into indices `0 .. framesToProcess - 1`. Each view
 *   spans the full render quantum size; only the first `framesToProcess`
 *   samples are read by native code.
 * @param outputChannelCount - Active output channel count (`audioData.length`).
 * @param framesToProcess - Number of frames to generate this quantum (may be
 *   less than the render quantum when playback starts or stops mid-buffer).
 * @param startOffset - Silent prefix length already zeroed in the output
 *   buffer; use when computing absolute sample time, not as a write offset into
 *   `audioData`.
 */
export type WorkletSourceNodeCallback = (
  audioData: Array<Float32Array>,
  outputChannelCount: number,
  framesToProcess: number,
  currentTime: number,
  startOffset: number
) => void;

/**
 * Invoked synchronously on the audio worklet runtime each render quantum.
 *
 * @param inputData - Stable per-channel `Float32Array` views over the input
 *   pool.
 * @param outputData - Stable per-channel `Float32Array` views over the output
 *   pool. Write processed samples here. Each view length equals the render
 *   quantum size.
 * @param inputChannelCount - Active input channel count (`inputData.length`).
 * @param outputChannelCount - Active output channel count
 *   (`outputData.length`).
 */
export type WorkletProcessingNodeCallback = (
  inputData: Array<Float32Array>,
  outputData: Array<Float32Array>,
  inputChannelCount: number,
  outputChannelCount: number,
  framesToProcess: number,
  currentTime: number
) => void;

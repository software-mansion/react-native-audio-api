package com.swmansion.audioapi.system

/**
 * Direct access to the active C++ recorder, independent of the JS runtime.
 * This is what allows the recording-notification stop action to end a recording
 * after the app task has been removed.
 */
object NativeRecorderControl {
  init {
    System.loadLibrary("react-native-audio-api")
  }

  /**
   * The file info is stashed natively for
   * `AudioRecorder.consumeLastRecordingResult()` on the JS side.
   */
  fun stop(): RecorderState = RecorderState.fromOrdinal(stopActiveRecording())

  /** Pauses an actively recording session; a no-op in any other state. */
  fun pause(): RecorderState = RecorderState.fromOrdinal(pauseActiveRecording())

  /** Resumes a paused session; a no-op in any other state. */
  fun resume(): RecorderState = RecorderState.fromOrdinal(resumeActiveRecording())

  /** Non-blocking read of the recorder state; [RecorderState.IDLE] when there is none. */
  fun currentState(): RecorderState = RecorderState.fromOrdinal(currentRecorderState())

  @JvmStatic
  private external fun stopActiveRecording(): Int

  @JvmStatic
  private external fun pauseActiveRecording(): Int

  @JvmStatic
  private external fun resumeActiveRecording(): Int

  @JvmStatic
  private external fun currentRecorderState(): Int
}

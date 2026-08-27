package com.swmansion.audioapi.system

/**
 * Direct access to the active C++ recorder, independent of the React context and the JS
 * runtime. This is what allows the recording-notification stop action to end a recording
 * after the app task has been removed.
 */
object NativeRecorderControl {
  init {
    System.loadLibrary("react-native-audio-api")
  }

  /**
   * Stops the active recording and finalizes its output file. Blocking — never call on
   * the main thread. The file info is stashed natively for
   * `AudioRecorder.takeLastRecordingResult()` on the JS side.
   *
   * @return true if a recording was stopped by this call.
   */
  @JvmStatic
  external fun stopActiveRecording(): Boolean

  /** Pauses an actively recording session. @return true if this call paused it. */
  @JvmStatic
  external fun pauseActiveRecording(): Boolean

  /** Resumes a paused session. @return true if this call resumed it. */
  @JvmStatic
  external fun resumeActiveRecording(): Boolean

  /** Non-blocking check whether a recording session (recording or paused) is ongoing. */
  @JvmStatic
  external fun isRecordingOngoing(): Boolean
}

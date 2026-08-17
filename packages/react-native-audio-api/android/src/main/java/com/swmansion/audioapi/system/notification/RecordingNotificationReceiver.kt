package com.swmansion.audioapi.system.notification

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.system.AudioEvent
import com.swmansion.audioapi.system.MediaSessionManager
import com.swmansion.audioapi.system.NativeRecorderControl
import java.util.concurrent.Executors

class RecordingNotificationReceiver(
  private val module: AudioAPIModule,
) : BroadcastReceiver() {
  companion object {
    const val ACTION_PAUSE = "com.swmansion.audioapi.RECORDING_NOTIFICATION_PAUSE"
    const val ACTION_RESUME = "com.swmansion.audioapi.RECORDING_NOTIFICATION_RESUME"
    const val ACTION_STOP = "com.swmansion.audioapi.RECORDING_NOTIFICATION_STOP"

    @Deprecated("Misleading name — it never stopped anything.", ReplaceWith("ACTION_PAUSE"))
    const val NOTIFICATION_RECORDING_STOPPED = ACTION_PAUSE

    @Deprecated("Renamed for consistency with the other actions.", ReplaceWith("ACTION_RESUME"))
    const val NOTIFICATION_RECORDING_RESUMED = ACTION_RESUME

    private const val TAG = "RecordingNotificationReceiver"

    private val controlExecutor = Executors.newSingleThreadExecutor()
  }

  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    when (intent?.action) {
      ACTION_PAUSE -> {
        togglePauseNatively(paused = true)
      }

      ACTION_RESUME -> {
        togglePauseNatively(paused = false)
      }

      ACTION_STOP -> {
        stopRecordingNatively()
      }
    }
  }

  /**
   * Every action acts on the recorder natively so the notification keeps working after
   * the app task was removed, when no JS listener is reachable. A live runtime is still
   * notified through the matching event so it can sync its UI; those handlers calling
   * the recorder again is harmless — the recorder ignores same-state transitions.
   *
   * Runs on an executor because [onReceive] is called on the main thread and the native
   * calls take the recorder's locks (stop even blocks on file finalization); [goAsync]
   * keeps the process alive meanwhile.
   */
  private fun togglePauseNatively(paused: Boolean) {
    val pendingResult = goAsync()
    controlExecutor.execute {
      try {
        if (paused) {
          NativeRecorderControl.pauseActiveRecording()
          module.invokeHandlerWithEventNameAndEventBody(AudioEvent.RECORDING_NOTIFICATION_PAUSE.ordinal, mapOf())
        } else {
          NativeRecorderControl.resumeActiveRecording()
          module.invokeHandlerWithEventNameAndEventBody(AudioEvent.RECORDING_NOTIFICATION_RESUME.ordinal, mapOf())
        }
        MediaSessionManager.setRecordingNotificationPaused(paused)
      } catch (e: UnsatisfiedLinkError) {
        Log.e(TAG, "Native library unavailable, cannot toggle the recording: ${e.message}", e)
      } catch (e: Exception) {
        Log.e(TAG, "Error while toggling the recording from the notification: ${e.message}", e)
      } finally {
        pendingResult.finish()
      }
    }
  }

  /** See [togglePauseNatively]; stopping additionally hides the notification, which lets
   * the foreground service unwind, and stashes the file info for
   * `AudioRecorder.takeLastRecordingResult()`. */
  private fun stopRecordingNatively() {
    val pendingResult = goAsync()
    controlExecutor.execute {
      try {
        NativeRecorderControl.stopActiveRecording()
        module.invokeHandlerWithEventNameAndEventBody(AudioEvent.RECORDING_NOTIFICATION_STOP.ordinal, mapOf())
        MediaSessionManager.hideRecordingNotification()
      } catch (e: UnsatisfiedLinkError) {
        Log.e(TAG, "Native library unavailable, cannot stop the recording: ${e.message}", e)
      } catch (e: Exception) {
        Log.e(TAG, "Error while stopping the recording from the notification: ${e.message}", e)
      } finally {
        pendingResult.finish()
      }
    }
  }
}

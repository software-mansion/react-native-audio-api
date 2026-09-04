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
        val toggled =
          if (paused) {
            NativeRecorderControl.pauseActiveRecording()
          } else {
            NativeRecorderControl.resumeActiveRecording()
          }
        // `false` means no recording was in a state this action applies to, so neither
        // the notification look nor JS may flip.
        if (toggled) {
          MediaSessionManager.setRecordingNotificationPaused(paused)
          dispatchEventToJs(
            if (paused) AudioEvent.RECORDING_NOTIFICATION_PAUSE else AudioEvent.RECORDING_NOTIFICATION_RESUME,
          )
        }
      } catch (e: LinkageError) {
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
        // The notification and foreground service unwind before JS is notified: the
        // recording is already over, so even a throwing JS dispatch must not leave a
        // stuck "recording" notification with a running microphone-typed service.
        MediaSessionManager.hideRecordingNotification()
        dispatchEventToJs(AudioEvent.RECORDING_NOTIFICATION_STOP)
      } catch (e: LinkageError) {
        Log.e(TAG, "Native library unavailable, cannot stop the recording: ${e.message}", e)
      } catch (e: Exception) {
        Log.e(TAG, "Error while stopping the recording from the notification: ${e.message}", e)
      } finally {
        pendingResult.finish()
      }
    }
  }

  /** Syncing a live JS runtime is best-effort — in the task-removed scenario the JNI
   * dispatch can throw, and that must not undo the native work that already completed. */
  private fun dispatchEventToJs(event: AudioEvent) {
    try {
      module.invokeHandlerWithEventNameAndEventBody(event.ordinal, mapOf())
    } catch (e: Exception) {
      Log.e(TAG, "Recording notification action completed natively, but notifying JS failed: ${e.message}", e)
    }
  }
}

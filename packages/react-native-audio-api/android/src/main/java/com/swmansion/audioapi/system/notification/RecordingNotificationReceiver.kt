package com.swmansion.audioapi.system.notification

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.system.AudioEvent
import com.swmansion.audioapi.system.MediaSessionManager
import com.swmansion.audioapi.system.NativeRecorderControl
import com.swmansion.audioapi.system.RecorderState
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
      ACTION_PAUSE ->
        applyToRecorder(
          action = NativeRecorderControl::pause,
          intendedState = RecorderState.PAUSED,
          event = AudioEvent.RECORDING_NOTIFICATION_PAUSE,
        )

      ACTION_RESUME ->
        applyToRecorder(
          action = NativeRecorderControl::resume,
          intendedState = RecorderState.RECORDING,
          event = AudioEvent.RECORDING_NOTIFICATION_RESUME,
        )

      ACTION_STOP ->
        applyToRecorder(
          action = NativeRecorderControl::stop,
          intendedState = RecorderState.IDLE,
          event = AudioEvent.RECORDING_NOTIFICATION_STOP,
        )
    }
  }

  /**
   * Every action acts on the recorder natively so the notification keeps working after
   * the app task was removed, when no JS listener is reachable.
   *
   * Runs on an executor because [onReceive] is called on the main thread and the native
   * calls take the recorder's locks (stop even blocks on file finalization); [goAsync]
   * keeps the process alive meanwhile.
   */
  private fun applyToRecorder(
    action: () -> RecorderState,
    intendedState: RecorderState,
    event: AudioEvent,
  ) {
    val pendingResult = goAsync()
    controlExecutor.execute {
      try {
        val state = action()
        renderNotification(state)
        if (state == intendedState) {
          dispatchEventToJs(event)
        }
      } catch (e: LinkageError) {
        Log.e(TAG, "Native library unavailable, cannot handle ${event.name}: ${e.message}", e)
      } catch (e: Exception) {
        Log.e(TAG, "Error while handling ${event.name} on the recorder: ${e.message}", e)
      } finally {
        pendingResult.finish()
      }
    }
  }

  private fun renderNotification(state: RecorderState) {
    when (state) {
      RecorderState.RECORDING -> MediaSessionManager.setRecordingNotificationPaused(false)
      RecorderState.PAUSED -> MediaSessionManager.setRecordingNotificationPaused(true)
      RecorderState.IDLE -> MediaSessionManager.hideRecordingNotification()
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

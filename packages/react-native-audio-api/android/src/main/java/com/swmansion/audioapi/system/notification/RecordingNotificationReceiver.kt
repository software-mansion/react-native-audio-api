package com.swmansion.audioapi.system.notification

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import com.swmansion.audioapi.AudioAPIModule

class RecordingNotificationReceiver(
  private val module: AudioAPIModule,
) : BroadcastReceiver() {
  companion object {
    const val NOTIFICATION_RECORDING_STOPPED = "com.swmansion.audioapi.NOTIFICATION_RECORDING_STOPPED"
    const val NOTIFICATION_RECORDING_RESUMED = "com.swmansion.audioapi.NOTIFICATION_RECORDING_RESUMED"
    private const val TAG = "RecordingNotificationReceiver"
  }

  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    when (intent?.action) {
      NOTIFICATION_RECORDING_STOPPED -> {
        Log.d(TAG, "Recording stopped via notification")
        module.invokeHandlerWithEventNameAndEventBody("recordingNotificationPause", mapOf())
      }

      NOTIFICATION_RECORDING_RESUMED -> {
        Log.d(TAG, "Recording resumed via notification")
        module.invokeHandlerWithEventNameAndEventBody("recordingNotificationResume", mapOf())
      }
    }
  }
}

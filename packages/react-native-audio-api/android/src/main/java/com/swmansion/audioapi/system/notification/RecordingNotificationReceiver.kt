package com.swmansion.audioapi.system.notification

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import com.swmansion.audioapi.AudioAPIModule

class RecordingNotificationReceiver(
  notification: RecordingNotification,
) : BroadcastReceiver() {
  companion object {
    const val NOTIFICATION_RECORDING_STOPPED = "com.swmansion.audioapi.NOTIFICATION_RECORDING_STOPPED"
    const val NOTIFICATION_RECORDING_RESUMED = "com.swmansion.audioapi.NOTIFICATION_RECORDING_RESUMED"
    private const val TAG = "RecordingNotificationReceiver"

    private var audioAPIModule: AudioAPIModule? = null

    fun setAudioAPIModule(module: AudioAPIModule?) {
      audioAPIModule = module
    }
  }

  private val notificationInstance: RecordingNotification = notification

  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    when (intent?.action) {
      NOTIFICATION_RECORDING_STOPPED -> {
        Log.d(TAG, "Recording stopped via notification")
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody("recordingNotificationPause", mapOf())
      }
      NOTIFICATION_RECORDING_RESUMED -> {
        Log.d(TAG, "Recording resumed via notification")
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody("recordingNotificationResume", mapOf())
      }
    }
  }
}

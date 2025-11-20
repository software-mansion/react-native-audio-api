package com.swmansion.audioapi.system.notification

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import com.swmansion.audioapi.AudioAPIModule

/**
 * Broadcast receiver for handling recording notification actions and dismissal.
 */
class RecordingNotificationReceiver : BroadcastReceiver() {
  companion object {
    const val ACTION_NOTIFICATION_DISMISSED = "com.swmansion.audioapi.RECORDING_NOTIFICATION_DISMISSED"
    private const val TAG = "RecordingNotificationReceiver"

    private var audioAPIModule: AudioAPIModule? = null

    fun setAudioAPIModule(module: AudioAPIModule?) {
      audioAPIModule = module
    }
  }

  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    when (intent?.action) {
      ACTION_NOTIFICATION_DISMISSED -> {
        Log.d(TAG, "Recording notification dismissed by user")
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody("recordingNotificationDismissed", mapOf())
      }
      RecordingNotification.ACTION_START -> {
        Log.d(TAG, "Start recording action received")
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody("recordingNotificationStart", mapOf())
      }
      RecordingNotification.ACTION_STOP -> {
        Log.d(TAG, "Stop recording action received")
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody("recordingNotificationStop", mapOf())
      }
    }
  }
}

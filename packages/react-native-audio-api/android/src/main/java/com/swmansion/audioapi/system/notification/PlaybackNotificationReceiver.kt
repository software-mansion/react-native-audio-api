package com.swmansion.audioapi.system.notification

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.system.AudioEvent

/**
 * Broadcast receiver for handling playback notification dismissal.
 */
class PlaybackNotificationReceiver : BroadcastReceiver() {
  companion object {
    const val ACTION_NOTIFICATION_DISMISSED = "com.swmansion.audioapi.PLAYBACK_NOTIFICATION_DISMISSED"
    const val ACTION_SKIP_FORWARD = "com.swmansion.audioapi.ACTION_SKIP_FORWARD"
    const val ACTION_SKIP_BACKWARD = "com.swmansion.audioapi.ACTION_SKIP_BACKWARD"

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
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_DISMISSED.ordinal, mapOf())
      }
      ACTION_SKIP_FORWARD -> {
        val body = HashMap<String, Any>().apply { put("value", 15) }
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_SKIP_FORWARD.ordinal, body)
      }
      ACTION_SKIP_BACKWARD -> {
        val body = HashMap<String, Any>().apply { put("value", 15) }
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_SKIP_BACKWARD.ordinal, body)
      }
    }
  }
}

package com.swmansion.audioapi.system.notification

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import com.swmansion.audioapi.AudioAPIModule

/**
 * Broadcast receiver for handling playback notification dismissal.
 */
class PlaybackNotificationReceiver : BroadcastReceiver() {
  companion object {
    const val ACTION_NOTIFICATION_DISMISSED = "com.swmansion.audioapi.PLAYBACK_NOTIFICATION_DISMISSED"
    private const val TAG = "PlaybackNotificationReceiver"

    private var audioAPIModule: AudioAPIModule? = null

    fun setAudioAPIModule(module: AudioAPIModule?) {
      audioAPIModule = module
    }
  }

  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    if (intent?.action == ACTION_NOTIFICATION_DISMISSED) {
      Log.d(TAG, "Notification dismissed by user")
      audioAPIModule?.invokeHandlerWithEventNameAndEventBody("playbackNotificationDismissed", mapOf())
    }
  }
}

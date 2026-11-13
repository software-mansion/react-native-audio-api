package com.swmansion.audioapi.system.notification

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.view.KeyEvent
import androidx.core.content.IntentCompat
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
    const val EXTRA_SKIP_INTERVAL_SECONDS = "skipIntervalSeconds"

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
        val skipInterval =
          intent.getIntExtra(
            EXTRA_SKIP_INTERVAL_SECONDS,
            PlaybackNotification.DEFAULT_SKIP_INTERVAL_SECONDS,
          )
        val body = HashMap<String, Any>().apply { put("value", skipInterval) }
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_SKIP_FORWARD.ordinal, body)
      }

      ACTION_SKIP_BACKWARD -> {
        val skipInterval =
          intent.getIntExtra(
            EXTRA_SKIP_INTERVAL_SECONDS,
            PlaybackNotification.DEFAULT_SKIP_INTERVAL_SECONDS,
          )
        val body = HashMap<String, Any>().apply { put("value", skipInterval) }
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_SKIP_BACKWARD.ordinal, body)
      }

      PlaybackNotification.MEDIA_BUTTON -> {
        val keyEvent = IntentCompat.getParcelableExtra(intent, Intent.EXTRA_KEY_EVENT, KeyEvent::class.java)
        val audioEvent =
          when (keyEvent?.keyCode) {
            KeyEvent.KEYCODE_MEDIA_PLAY -> AudioEvent.PLAYBACK_NOTIFICATION_PLAY
            KeyEvent.KEYCODE_MEDIA_PAUSE -> AudioEvent.PLAYBACK_NOTIFICATION_PAUSE
            KeyEvent.KEYCODE_MEDIA_STOP -> AudioEvent.PLAYBACK_NOTIFICATION_STOP
            KeyEvent.KEYCODE_MEDIA_NEXT -> AudioEvent.PLAYBACK_NOTIFICATION_NEXT_TRACK
            KeyEvent.KEYCODE_MEDIA_PREVIOUS -> AudioEvent.PLAYBACK_NOTIFICATION_PREVIOUS_TRACK
            else -> return
          }
        audioAPIModule?.invokeHandlerWithEventNameAndEventBody(audioEvent.ordinal, mapOf())
      }
    }
  }
}

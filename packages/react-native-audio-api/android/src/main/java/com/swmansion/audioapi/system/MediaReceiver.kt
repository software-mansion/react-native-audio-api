package com.swmansion.audioapi.system

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.media.AudioManager
import android.view.KeyEvent
import com.facebook.react.bridge.ReactApplicationContext

class MediaReceiver(
  val reactContext: ReactApplicationContext,
  private val mediaSessionManager: MediaSessionManager,
) : BroadcastReceiver() {
  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    val action = intent!!.action

    if (MediaNotificationManager.REMOVE_NOTIFICATION == action) {
      if (!checkApp(intent)) return

      // removes the notification and deactivates the media session
      mediaSessionManager.mediaNotificationManager.hide()
      mediaSessionManager.mediaSession.isActive = false

      mediaSessionManager.eventEmitter.sendEvent("onCloseNotification", null, null)
    } else if (MediaNotificationManager.MEDIA_BUTTON == action || Intent.ACTION_MEDIA_BUTTON == action) {
      if (!intent.hasExtra(Intent.EXTRA_KEY_EVENT)) return
      if (!checkApp(intent)) return

      val keyEvent = intent.getParcelableExtra<KeyEvent>(Intent.EXTRA_KEY_EVENT)
      mediaSessionManager.mediaSession.controller.dispatchMediaButtonEvent(keyEvent)
    } else if (AudioManager.ACTION_AUDIO_BECOMING_NOISY == action) {
      mediaSessionManager.mediaSession.controller.transportControls
        .pause()
    }
  }

  private fun checkApp(intent: Intent): Boolean {
    if (intent.hasExtra(MediaNotificationManager.PACKAGE_NAME)) {
      val name = intent.getStringExtra(MediaNotificationManager.PACKAGE_NAME)
      if (!reactContext.packageName.equals(name)) return false
    }
    return true
  }
}

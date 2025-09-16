package com.swmansion.audioapi.system

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.media.AudioManager
import android.support.v4.media.session.MediaSessionCompat
import android.view.KeyEvent
import com.facebook.react.bridge.ReactApplicationContext
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference

class MediaReceiver(
  private val reactContext: WeakReference<ReactApplicationContext>,
  private val mediaSession: WeakReference<MediaSessionCompat>,
  private val mediaNotificationManager: WeakReference<MediaNotificationManager>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
) : BroadcastReceiver() {
  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    val action = intent!!.action

    if (MediaNotificationManager.REMOVE_NOTIFICATION == action) {
      if (!checkApp(intent)) return

      mediaNotificationManager.get()?.cancelNotification()
      MediaSessionManager.stopForegroundServiceIfNecessary()
      mediaSession.get()?.isActive = false

      audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("closeNotification", mapOf()) // add to ts events
    } else if (MediaNotificationManager.MEDIA_BUTTON == action || Intent.ACTION_MEDIA_BUTTON == action) {
      if (!intent.hasExtra(Intent.EXTRA_KEY_EVENT)) return
      if (!checkApp(intent)) return

      val keyEvent = intent.getParcelableExtra<KeyEvent>(Intent.EXTRA_KEY_EVENT)
      mediaSession.get()?.controller?.dispatchMediaButtonEvent(keyEvent)
    } else if (AudioManager.ACTION_AUDIO_BECOMING_NOISY == action) {
      mediaSession
        .get()
        ?.controller
        ?.transportControls
        ?.pause()
    }
  }

  private fun checkApp(intent: Intent): Boolean {
    if (intent.hasExtra(MediaNotificationManager.PACKAGE_NAME)) {
      val name = intent.getStringExtra(MediaNotificationManager.PACKAGE_NAME)
      if (!reactContext.get()?.packageName.equals(name)) {
        return false
      }
    }
    return true
  }
}

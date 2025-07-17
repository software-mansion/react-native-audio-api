package com.swmansion.audioapi.system

import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.support.v4.media.session.MediaSessionCompat
import android.util.Log
import androidx.core.app.NotificationManagerCompat
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference
import java.util.HashMap

class MediaSessionCallback(
  private val audioAPIModule: WeakReference<AudioAPIModule>,
) : MediaSessionCompat.Callback() {
  override fun onPlay() {
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remotePlay", mapOf())
  }

  override fun onPause() {
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remotePause", mapOf())
  }

  override fun onStop() {
    val reactContext = audioAPIModule.get()?.reactContext?.get()!!
    NotificationManagerCompat.from(reactContext).cancel(MediaSessionManager.NOTIFICATION_ID)
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val myIntent =
        Intent(reactContext, MediaNotificationManager.NotificationService::class.java)
      reactContext.stopService(myIntent)
    }

    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remoteStop", mapOf())
  }

  override fun onSkipToNext() {
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remoteNextTrack", mapOf())
  }

  override fun onSkipToPrevious() {
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remotePreviousTrack", mapOf())
  }

  override fun onFastForward() {
    val body = HashMap<String, Any>().apply { put("value", 15) }
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remoteSkipForward", body)
  }

  override fun onRewind() {
    val body = HashMap<String, Any>().apply { put("value", 15) }
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remoteSkipBackward", body)
  }

  override fun onSeekTo(pos: Long) {
    val body = HashMap<String, Any>().apply { put("value", (pos.toDouble() / 1000)) }
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remoteChangePlaybackPosition", body)
  }

  override fun onCustomAction(
    action: String?,
    extras: Bundle?,
  ) {
    when (action) {
      "SkipForward" -> onFastForward()
      "SkipBackward" -> onRewind()
      else -> Log.w("MediaSessionCallback", "Unknown custom action: $action")
    }
  }
}

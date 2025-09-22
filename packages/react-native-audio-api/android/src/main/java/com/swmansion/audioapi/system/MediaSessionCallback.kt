package com.swmansion.audioapi.system

import android.os.Bundle
import android.support.v4.media.session.MediaSessionCompat
import android.util.Log
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference

class MediaSessionCallback(
  private val audioAPIModule: WeakReference<AudioAPIModule>,
  private val mediaNotificationManager: WeakReference<MediaNotificationManager>,
) : MediaSessionCompat.Callback() {
  override fun onPlay() {
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remotePlay", mapOf())
  }

  override fun onPause() {
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remotePause", mapOf())
  }

  override fun onStop() {
    mediaNotificationManager.get()?.cancelNotification()
    MediaSessionManager.stopForegroundServiceIfNecessary()

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

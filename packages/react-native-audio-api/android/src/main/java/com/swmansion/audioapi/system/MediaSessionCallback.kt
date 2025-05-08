package com.swmansion.audioapi.system

import android.content.Intent
import android.os.Build
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import androidx.core.app.NotificationManagerCompat
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference
import java.util.HashMap

class MediaSessionCallback(
  audioAPIModule: AudioAPIModule,
  private val lockScreenManager: LockScreenManager,
) : MediaSessionCompat.Callback() {
  private val audioAPIModule: WeakReference<AudioAPIModule> = WeakReference(audioAPIModule)

  override fun onPlay() {
    lockScreenManager.updatePlaybackState(PlaybackStateCompat.STATE_PLAYING)
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remotePlay", mapOf())
  }

  override fun onPause() {
    lockScreenManager.updatePlaybackState(PlaybackStateCompat.STATE_PAUSED)
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
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remoteSkipForward", mapOf())
  }

  override fun onRewind() {
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remoteSkipBackward", mapOf())
  }

  override fun onSeekTo(pos: Long) {
    val body = HashMap<String, Any>().apply { put("value", (pos.toDouble() / 1000)) }
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("remoteChangePlaybackPosition", body)
  }
}

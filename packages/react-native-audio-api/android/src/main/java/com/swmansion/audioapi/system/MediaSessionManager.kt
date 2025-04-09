package com.swmansion.audioapi.system

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.os.Build
import android.support.v4.media.session.MediaSessionCompat
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap

class MediaSessionManager(
  val reactContext: ReactApplicationContext,
) {
  val notificationId = 100
  val channelId = "react-native-audio-api"

  private val mediaSession: MediaSessionCompat
  private val mediaNotificationManager: MediaNotificationManager
  private val lockScreenManager: LockScreenManager

  init {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      createChannel()
    }

    this.mediaSession = MediaSessionCompat(reactContext, "MediaSessionManager")
    this.mediaNotificationManager = MediaNotificationManager(reactContext, notificationId)
    this.lockScreenManager = LockScreenManager(reactContext, mediaSession, mediaNotificationManager, channelId)
  }

  fun setLockScreenInfo(info: ReadableMap?) {
    lockScreenManager.setLockScreenInfo(info)
  }

  fun resetLockScreenInfo() {
    lockScreenManager.resetLockScreenInfo()
  }

  fun enableRemoteCommand(
    name: String,
    enabled: Boolean,
  ) {
    lockScreenManager.enableRemoteCommand(name, enabled)
  }

  @RequiresApi(Build.VERSION_CODES.O)
  private fun createChannel() {
    val notificationManager =
      reactContext.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

    val mChannel =
      NotificationChannel(channelId, "Audio manager", NotificationManager.IMPORTANCE_LOW)
    mChannel.description = "Audio manager"
    mChannel.setShowBadge(false)
    mChannel.lockscreenVisibility = NotificationCompat.VISIBILITY_PUBLIC
    notificationManager.createNotificationChannel(mChannel)
  }
}

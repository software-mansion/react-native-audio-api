package com.swmansion.audioapi.system

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import com.swmansion.audioapi.system.MediaSessionManager.CHANNEL_ID
import com.swmansion.audioapi.system.notification.NotificationRegistry
import com.swmansion.audioapi.system.notification.PlaybackNotification
import com.swmansion.audioapi.system.notification.RecordingNotification

/**
 * Centralized foreground service that can be used by any component that needs foreground capabilities.
 */
class CentralizedForegroundService : Service() {
  companion object {
    private const val TAG = "CentralizedForegroundService"
    const val ACTION_START = "START_FOREGROUND"
    const val ACTION_STOP = "STOP_FOREGROUND"
  }

  override fun onBind(intent: Intent?): IBinder? = null

  override fun onStartCommand(
    intent: Intent?,
    flags: Int,
    startId: Int,
  ): Int {
    when (intent?.action) {
      ACTION_START -> {
        startForegroundWithNotification()
      }

      ACTION_STOP -> {
        stopForeground(STOP_FOREGROUND_REMOVE)
        stopSelf()
      }
    }
    return START_NOT_STICKY
  }

  private fun startForegroundWithNotification() {
    try {
      createNotificationChannelIfNeeded()

      // Get the first available notification
      val existingNotification = findExistingNotification()
      if (existingNotification == null) {
        Log.w(TAG, "No notification available to start foreground service")
        return
      }

      val (notificationId, notification) = existingNotification

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        startForeground(
          notificationId,
          notification,
        )
      } else {
        startForeground(notificationId, notification)
      }

      Log.d(TAG, "Centralized foreground service started with notification ID: $notificationId")
    } catch (e: Exception) {
      Log.e(TAG, "Error starting foreground service: ${e.message}", e)
    }
  }

  private fun findExistingNotification(): Pair<Int, Notification>? {
    // Check for playback notification first (priority)
    NotificationRegistry.getBuiltNotification(PlaybackNotification.ID)?.let {
      return PlaybackNotification.ID to it
    }

    NotificationRegistry.getBuiltNotification(RecordingNotification.ID)?.let {
      return RecordingNotification.ID to it
    }

    return null
  }

  private fun createNotificationChannelIfNeeded() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

      if (notificationManager.getNotificationChannel(CHANNEL_ID) == null) {
        val channel =
          NotificationChannel(
            CHANNEL_ID,
            "Audio Service",
            NotificationManager.IMPORTANCE_LOW,
          ).apply {
            description = "Background audio processing"
            setShowBadge(false)
            lockscreenVisibility = NotificationCompat.VISIBILITY_PUBLIC
          }
        notificationManager.createNotificationChannel(channel)
      }
    }
  }

  override fun onDestroy() {
    Log.d(TAG, "Centralized foreground service destroyed")
    super.onDestroy()
  }
}

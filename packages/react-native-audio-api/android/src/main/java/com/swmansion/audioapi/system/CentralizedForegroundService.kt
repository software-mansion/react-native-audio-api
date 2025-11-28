package com.swmansion.audioapi.system

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import com.swmansion.audioapi.system.MediaSessionManager.CHANNEL_ID
import com.swmansion.audioapi.system.notification.NotificationRegistry

/**
 * Centralized foreground service that can be used by any component that needs foreground capabilities.
 */
class CentralizedForegroundService : Service() {
  companion object {
    private const val TAG = "CentralizedForegroundService"
    private const val NOTIFICATION_ID = 100
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

      // Try to use an existing notification first
      val existingNotification = findExistingNotification()
      val (notificationId, notification) =
        if (existingNotification != null) {
          existingNotification
        } else {
          // Fallback to default service notification
          NOTIFICATION_ID to createServiceNotification()
        }

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        startForeground(
          notificationId,
          notification,
          ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK,
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
    // Check for recording notification first (priority)
    NotificationRegistry.getBuiltNotification(101)?.let {
      return 101 to it
    }

    // Check for playback notification
    NotificationRegistry.getBuiltNotification(100)?.let {
      return 100 to it
    }

    return null
  }

  private fun createServiceNotification(): Notification =
    NotificationCompat
      .Builder(this, CHANNEL_ID)
      .setContentTitle("Audio Service")
      .setContentText("Audio processing in progress")
      .setSmallIcon(android.R.drawable.ic_btn_speak_now)
      .setPriority(NotificationCompat.PRIORITY_LOW)
      .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
      .setOngoing(true)
      .setAutoCancel(false)
      .build()

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

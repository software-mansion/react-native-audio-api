package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.util.Log

/**
 * Foreground service for displaying persistent notifications.
 */
class ForegroundNotificationService : Service() {
  companion object {
    private const val TAG = "ForegroundNotifService"
    const val ACTION_START_FOREGROUND = "START_FOREGROUND"
    const val ACTION_STOP_FOREGROUND = "STOP_FOREGROUND"
    const val EXTRA_NOTIFICATION_ID = "notification_id"
    const val EXTRA_NOTIFICATION_KEY = "notification_key"
  }

  private var isServiceStarted = false
  private val serviceLock = Any()

  override fun onBind(intent: Intent?): IBinder? = null

  override fun onStartCommand(
    intent: Intent?,
    flags: Int,
    startId: Int,
  ): Int {
    when (intent?.action) {
      ACTION_START_FOREGROUND -> {
        val notificationId = intent.getIntExtra(EXTRA_NOTIFICATION_ID, -1)
        val notificationKey = intent.getStringExtra(EXTRA_NOTIFICATION_KEY)

        if (notificationId != -1 && notificationKey != null) {
          startForegroundService(notificationId, notificationKey)
        } else {
          Log.w(TAG, "Invalid notification data received")
        }
      }
      ACTION_STOP_FOREGROUND -> {
        stopForegroundService()
      }
      else -> {
        Log.w(TAG, "Unknown action: ${intent?.action}")
      }
    }

    return START_NOT_STICKY
  }

  private fun startForegroundService(
    notificationId: Int,
    notificationKey: String,
  ) {
    synchronized(serviceLock) {
      if (!isServiceStarted) {
        try {
          // TODO retrieve actual notification from NotificationRegistry
          val notification = createDummyNotification(notificationId)

          if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(
              notificationId,
              notification,
              ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK,
            )
          } else {
            startForeground(notificationId, notification)
          }

          isServiceStarted = true
          Log.d(TAG, "Foreground service started with notification: $notificationKey")
        } catch (e: Exception) {
          Log.e(TAG, "Error starting foreground service: ${e.message}", e)
        }
      }
    }
  }

  private fun stopForegroundService() {
    synchronized(serviceLock) {
      if (isServiceStarted) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
          stopForeground(STOP_FOREGROUND_REMOVE)
        }
        isServiceStarted = false
        stopSelf()
        Log.d(TAG, "Foreground service stopped")
      }
    }
  }

  override fun onTaskRemoved(rootIntent: Intent?) {
    super.onTaskRemoved(rootIntent)
    stopForegroundService()
  }

  override fun onDestroy() {
    synchronized(serviceLock) {
      isServiceStarted = false
    }
    super.onDestroy()
  }

  private fun createDummyNotification(notificationId: Int): Notification {
    // TODO This is a placeholder - in a real implementation, we'd need to pass
    // the actual notification or retrieve it from a shared location
    return Notification
      .Builder(this, SimpleNotification.CHANNEL_ID)
      .setContentTitle("Audio Service")
      .setContentText("Running in background")
      .setSmallIcon(android.R.drawable.ic_media_play)
      .build()
  }
}

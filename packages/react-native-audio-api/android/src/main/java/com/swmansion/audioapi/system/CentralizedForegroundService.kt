package com.swmansion.audioapi.system

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.annotation.RequiresApi
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

    private const val PLACEHOLDER_CHANNEL_ID = "audio_service_placeholder"
    private const val PLACEHOLDER_NOTIFICATION_ID = 300
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

  override fun onTaskRemoved(rootIntent: Intent?) {
    // Fires only when the app opted into android:stopWithTask="false" — the service (and any
    // in-progress recording or playback) intentionally outlives the removed task.
    Log.i(TAG, "App task removed, foreground service keeps running")
    super.onTaskRemoved(rootIntent)
  }

  private fun startForegroundWithNotification() {
    try {
      createLowImportanceChannelIfNeeded(CHANNEL_ID, "Audio Service", "Background audio processing")

      // Get the first available notification
      val existingNotification = findExistingNotification()
      if (existingNotification == null) {
        // The service was started with Context.startForegroundService(), so startForeground()
        // must still be called — skipping it crashes with ForegroundServiceDidNotStartInTimeException.
        Log.w(TAG, "No notification available, starting foreground with a placeholder and stopping")
        startForegroundWithPlaceholderAndStop()
        return
      }

      val (notificationId, notification) = existingNotification
      startForegroundCompat(notificationId, notification)

      Log.d(TAG, "Centralized foreground service started with notification ID: $notificationId")
    } catch (e: Exception) {
      Log.e(TAG, "Error starting foreground service: ${e.message}", e)
    }
  }

  private fun startForegroundWithPlaceholderAndStop() {
    createLowImportanceChannelIfNeeded(
      PLACEHOLDER_CHANNEL_ID,
      "Audio Service Placeholder",
      "Short-lived notification shown while the audio service shuts down",
    )

    val placeholderNotification =
      NotificationCompat
        .Builder(this, PLACEHOLDER_CHANNEL_ID)
        .setSmallIcon(android.R.drawable.ic_media_play)
        .setContentTitle("Audio service")
        .setPriority(NotificationCompat.PRIORITY_LOW)
        .build()

    try {
      startForegroundCompat(PLACEHOLDER_NOTIFICATION_ID, placeholderNotification)
    } finally {
      // The service must exit even when startForeground throws (e.g. API 34+
      // ForegroundServiceStartNotAllowedException) — otherwise the system kills the
      // process with ForegroundServiceDidNotStartInTimeException.
      stopForeground(STOP_FOREGROUND_REMOVE)
      stopSelf()
    }
  }

  private fun startForegroundCompat(
    notificationId: Int,
    notification: Notification,
  ) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
      startForeground(notificationId, notification)
      return
    }

    // Passing a type the app did not declare in its manifest throws, so only the intersection
    // of desired and declared types may be used.
    val serviceTypes = activeNotificationServiceTypes() and manifestDeclaredServiceTypes()
    when {
      serviceTypes != 0 -> {
        startForeground(notificationId, notification, serviceTypes)
      }

      Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE -> {
        startForeground(notificationId, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MANIFEST)
      }

      else -> {
        startForeground(notificationId, notification)
      }
    }
  }

  @RequiresApi(Build.VERSION_CODES.Q)
  private fun activeNotificationServiceTypes(): Int {
    var serviceTypes = 0

    if (NotificationRegistry.getBuiltNotification(PlaybackNotification.ID) != null) {
      serviceTypes = serviceTypes or ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R &&
      NotificationRegistry.getBuiltNotification(RecordingNotification.ID) != null
    ) {
      serviceTypes = serviceTypes or ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE
    }

    return serviceTypes
  }

  @RequiresApi(Build.VERSION_CODES.Q)
  private fun manifestDeclaredServiceTypes(): Int =
    try {
      packageManager
        .getServiceInfo(ComponentName(this, CentralizedForegroundService::class.java), PackageManager.GET_META_DATA)
        .foregroundServiceType
    } catch (e: PackageManager.NameNotFoundException) {
      Log.w(TAG, "Unable to read foreground service types declared in the manifest: ${e.message}")
      0
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

  private fun createLowImportanceChannelIfNeeded(
    id: String,
    name: String,
    channelDescription: String,
  ) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
      return
    }

    val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    if (notificationManager.getNotificationChannel(id) != null) {
      return
    }

    val channel =
      NotificationChannel(
        id,
        name,
        NotificationManager.IMPORTANCE_LOW,
      ).apply {
        description = channelDescription
        setShowBadge(false)
        lockscreenVisibility = NotificationCompat.VISIBILITY_PUBLIC
      }
    notificationManager.createNotificationChannel(channel)
  }

  override fun onDestroy() {
    Log.d(TAG, "Centralized foreground service destroyed")
    ForegroundServiceManager.onServiceDestroyed()
    super.onDestroy()
  }
}

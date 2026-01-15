package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.util.Log
import androidx.annotation.RequiresPermission
import androidx.core.app.NotificationManagerCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.system.ForegroundServiceManager
import java.lang.ref.WeakReference

/**
 * Central notification registry that manages multiple notification instances.
 * Automatically handles foreground service lifecycle based on active notifications.
 */
class NotificationRegistry(
  private val reactContext: WeakReference<ReactApplicationContext>,
  private val audioAPIModule: WeakReference<com.swmansion.audioapi.AudioAPIModule>,
) {
  companion object {
    private const val TAG = "NotificationRegistry"

    // Store last built notifications for foreground service access
    private val builtNotifications = mutableMapOf<Int, Notification>()

    fun getBuiltNotification(notificationId: Int): Notification? = builtNotifications[notificationId]
  }

  private val notifications = mutableMapOf<String, BaseNotification>()
  private val activeNotifications = mutableMapOf<String, Boolean>()

  /**
   * Show or update a notification.
   * Automatically creates the notification instance on first call.
   * If notification is not visible, it will be shown and subscribed to foreground service.
   * If already visible, it will be updated.
   *
   * @param key The unique identifier of the notification
   * @param type The type of notification (only used for first creation)
   * @param options Configuration options from JavaScript
   */
  @RequiresPermission(android.Manifest.permission.POST_NOTIFICATIONS)
  fun showNotification(
    key: String,
    type: String,
    options: ReadableMap?,
  ) {
    // Auto-create notification if it doesn't exist
    if (!notifications.containsKey(key)) {
      createNotification(type, key)
    }

    val notification = notifications[key]
    if (notification == null) {
      Log.w(TAG, "Notification not found: $key")
      return
    }

    try {
      val wasActive = isNotificationActive(key)

      // Build/update the notification
      val builtNotification = notification.show(options)
      displayNotification(notification.getNotificationId(), builtNotification)

      // Subscribe to foreground service if not already active
      if (!wasActive) {
        ForegroundServiceManager.subscribe(notification)
        activeNotifications[key] = true
        Log.d(TAG, "Showing notification: $key (subscribed to foreground service)")
      } else {
        Log.d(TAG, "Updating notification: $key")
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error showing notification $key: ${e.message}", e)
    }
  }

  /**
   * Hide a notification.
   *
   * @param key The unique identifier of the notification
   */
  fun hideNotification(key: String) {
    val notification = notifications[key]
    if (notification == null) {
      // Silently ignore if notification doesn't exist
      return
    }

    try {
      // Only hide if currently active
      if (activeNotifications.getOrDefault(key, false)) {
        cancelNotification(notification.getNotificationId())
        notification.hide()
        activeNotifications[key] = false

        // Unsubscribe from foreground service
        ForegroundServiceManager.unsubscribe(notification)

        Log.d(TAG, "Hiding notification: $key (unsubscribed from foreground service)")
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error hiding notification $key: ${e.message}", e)
    }
  }

  /**
   * Create a notification instance.
   *
   * @param type The type of notification to create
   * @param key Unique identifier for this notification
   */
  private fun createNotification(
    type: String,
    key: String,
  ) {
    val notification =
      when (type) {
        "playback" -> {
          PlaybackNotification(
            reactContext,
            audioAPIModule,
            PlaybackNotification.ID,
            "audio_playback",
          )
        }

        "recording" -> {
          RecordingNotification(
            reactContext,
            audioAPIModule,
            RecordingNotification.ID,
            "audio_recording4",
          )
        }

        else -> {
          throw IllegalArgumentException("Unknown notification type: $type")
        }
      }

    notifications[key] = notification
    Log.d(TAG, "Created notification: $key (type: $type)")
  }

  /**
   * Destroy and cleanup a notification.
   *
   * @param key The unique identifier of the notification
   */
  fun destroyNotification(key: String) {
    hideNotification(key)
    notifications.remove(key)
    activeNotifications.remove(key)
    Log.d(TAG, "Destroyed notification: $key")
  }

  /**
   * Check if a notification is currently active.
   */
  fun isNotificationActive(key: String): Boolean = activeNotifications.getOrDefault(key, false)

  /**
   * Get all registered notification keys.
   */
  fun getRegisteredKeys(): Set<String> = notifications.keys.toSet()

  /**
   * Cleanup all notifications.
   */
  fun cleanup() {
    notifications.keys.toList().forEach { key ->
      hideNotification(key)
    }
    notifications.clear()
    activeNotifications.clear()
    builtNotifications.clear()

    // Cleanup foreground service manager
    ForegroundServiceManager.cleanup()

    Log.d(TAG, "Cleaned up all notifications")
  }

  @RequiresPermission(android.Manifest.permission.POST_NOTIFICATIONS)
  private fun displayNotification(
    id: Int,
    notification: Notification,
  ) {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    Log.d(TAG, "Displaying notification with ID: $id")
    try {
      // Store notification for foreground service access
      builtNotifications[id] = notification

      NotificationManagerCompat.from(context).notify(id, notification)
      Log.d(TAG, "Notification posted successfully with ID: $id")
    } catch (e: Exception) {
      Log.e(TAG, "Error posting notification: ${e.message}", e)
    }
  }

  private fun cancelNotification(id: Int) {
    val context = reactContext.get() ?: return
    NotificationManagerCompat.from(context).cancel(id)
    // Clean up stored notification
    builtNotifications.remove(id)
  }
}

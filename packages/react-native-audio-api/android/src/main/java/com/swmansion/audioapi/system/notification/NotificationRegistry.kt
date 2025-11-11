package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.util.Log
import androidx.core.app.NotificationManagerCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import java.lang.ref.WeakReference

/**
 * Central notification registry that manages multiple notification instances.
 */
class NotificationRegistry(
  private val reactContext: WeakReference<ReactApplicationContext>,
) {
  companion object {
    private const val TAG = "NotificationRegistry"

    // Store last built notifications for foreground service access
    private val builtNotifications = mutableMapOf<Int, Notification>()

    fun getBuiltNotification(notificationId: Int): Notification? {
      return builtNotifications[notificationId]
    }
  }

  private val notifications = mutableMapOf<String, BaseNotification>()
  private val activeNotifications = mutableMapOf<String, Boolean>()

  /**
   * Register a new notification instance.
   *
   * @param key Unique identifier for this notification
   * @param notification The notification instance to register
   */
  fun registerNotification(
    key: String,
    notification: BaseNotification,
  ) {
    notifications[key] = notification
    Log.d(TAG, "Registered notification: $key")
  }

  /**
   * Initialize and show a notification.
   *
   * @param key The unique identifier of the notification
   * @param options Configuration options from JavaScript
   */
  fun showNotification(
    key: String,
    options: ReadableMap?,
  ) {
    val notification = notifications[key]
    if (notification == null) {
      Log.w(TAG, "Notification not found: $key")
      return
    }

    try {
      val builtNotification = notification.init(options)
      displayNotification(notification.getNotificationId(), builtNotification)
      activeNotifications[key] = true
      Log.d(TAG, "Showing notification: $key")
    } catch (e: Exception) {
      Log.e(TAG, "Error showing notification $key: ${e.message}", e)
    }
  }

  /**
   * Update an existing notification with new options.
   *
   * @param key The unique identifier of the notification
   * @param options New configuration options from JavaScript
   */
  fun updateNotification(
    key: String,
    options: ReadableMap?,
  ) {
    if (!activeNotifications.getOrDefault(key, false)) {
      Log.w(TAG, "Cannot update inactive notification: $key")
      return
    }

    val notification = notifications[key]
    if (notification == null) {
      Log.w(TAG, "Notification not found: $key")
      return
    }

    try {
      val builtNotification = notification.update(options)
      displayNotification(notification.getNotificationId(), builtNotification)
      Log.d(TAG, "Updated notification: $key")
    } catch (e: Exception) {
      Log.e(TAG, "Error updating notification $key: ${e.message}", e)
    }
  }

  /**
   * Hide and reset a notification.
   *
   * @param key The unique identifier of the notification
   */
  fun hideNotification(key: String) {
    val notification = notifications[key]
    if (notification == null) {
      Log.w(TAG, "Notification not found: $key")
      return
    }

    try {
      cancelNotification(notification.getNotificationId())
      notification.reset()
      activeNotifications[key] = false
      Log.d(TAG, "Hiding notification: $key")
    } catch (e: Exception) {
      Log.e(TAG, "Error hiding notification $key: ${e.message}", e)
    }
  }

  /**
   * Unregister and cleanup a notification.
   *
   * @param key The unique identifier of the notification
   */
  fun unregisterNotification(key: String) {
    hideNotification(key)
    notifications.remove(key)
    activeNotifications.remove(key)
    Log.d(TAG, "Unregistered notification: $key")
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
    Log.d(TAG, "Cleaned up all notifications")
  }

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

  /**
   * Start foreground service with the given notification.
   * This is required for Android O+ when app is in background.
   *
   * @param key The notification key to use for foreground service
   */
  fun startForegroundService(key: String) {
    val notification = notifications[key]
    if (notification == null) {
      Log.w(TAG, "Cannot start foreground service: notification $key not found")
      return
    }

    val context = reactContext.get() ?: return
    val intent = Intent(context, ForegroundNotificationService::class.java)
    intent.action = ForegroundNotificationService.ACTION_START_FOREGROUND
    intent.putExtra(ForegroundNotificationService.EXTRA_NOTIFICATION_ID, notification.getNotificationId())
    intent.putExtra(ForegroundNotificationService.EXTRA_NOTIFICATION_KEY, key)

    try {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        context.startForegroundService(intent)
      } else {
        context.startService(intent)
      }
      Log.d(TAG, "Started foreground service with notification: $key")
    } catch (e: Exception) {
      Log.e(TAG, "Error starting foreground service: ${e.message}", e)
    }
  }

  /**
   * Stop the foreground service.
   */
  fun stopForegroundService() {
    val context = reactContext.get() ?: return
    val intent = Intent(context, ForegroundNotificationService::class.java)
    intent.action = ForegroundNotificationService.ACTION_STOP_FOREGROUND

    try {
      context.startService(intent)
      Log.d(TAG, "Stopped foreground service")
    } catch (e: Exception) {
      Log.e(TAG, "Error stopping foreground service: ${e.message}", e)
    }
  }
}

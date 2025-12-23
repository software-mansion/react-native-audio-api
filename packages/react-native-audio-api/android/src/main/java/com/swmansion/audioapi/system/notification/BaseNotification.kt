package com.swmansion.audioapi.system.notification

import android.app.Notification
import com.facebook.react.bridge.ReadableMap

/**
 * Base interface for all notification types.
 * Implementations should handle their own notification channel creation,
 * notification building, and lifecycle management.
 */
interface BaseNotification {
  /**
   * Show or update the notification with the provided options.
   * This method should create/update the notification and prepare it for display.
   * It handles both initial display and updates.
   *
   * @param options Configuration options from JavaScript side
   * @return The built Notification ready to be shown
   */
  fun show(options: ReadableMap?): Notification

  /**
   * Hide the notification and cleanup resources.
   * This should clear any stored data and stop any ongoing processes.
   */
  fun hide()

  /**
   * Get the unique ID for this notification.
   * Used by the NotificationManager to track and manage notifications.
   */
  fun getNotificationId(): Int

  /**
   * Get the channel ID for this notification.
   * Required for Android O+ notification channels.
   */
  fun getChannelId(): String
}

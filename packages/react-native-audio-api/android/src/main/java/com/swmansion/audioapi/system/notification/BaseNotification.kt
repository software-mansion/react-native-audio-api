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
   * Initialize the notification with the provided options.
   * This method should create the notification and prepare it for display.
   *
   * @param options Configuration options from JavaScript side
   * @return The built Notification ready to be shown
   */
  fun init(options: ReadableMap?): Notification

  /**
   * Reset the notification to its initial state.
   * This should clear any stored data and stop any ongoing processes.
   */
  fun reset()

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

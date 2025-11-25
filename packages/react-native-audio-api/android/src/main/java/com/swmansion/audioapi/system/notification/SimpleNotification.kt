package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.os.Build
import androidx.core.app.NotificationCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.R
import java.lang.ref.WeakReference

/**
 * This serves as a reference implementation and starting point for more complex notifications.
 */
class SimpleNotification(
  private val reactContext: WeakReference<ReactApplicationContext>,
  private val notificationId: Int = DEFAULT_NOTIFICATION_ID,
) : BaseNotification {
  companion object {
    const val DEFAULT_NOTIFICATION_ID = 200
    const val CHANNEL_ID = "react-native-audio-api-simple"
    const val CHANNEL_NAME = "Simple Notifications"
  }

  private var title: String = "Audio Playing"
  private var text: String = ""

  init {
    createNotificationChannel()
  }

  override fun init(options: ReadableMap?): Notification {
    // Parse options from JavaScript
    if (options != null) {
      if (options.hasKey("title")) {
        title = options.getString("title") ?: "Audio Playing"
      }
      if (options.hasKey("text")) {
        text = options.getString("text") ?: ""
      }
    }

    return buildNotification()
  }

  override fun update(options: ReadableMap?): Notification {
    // Update works the same as init for simple notifications
    return init(options)
  }

  override fun reset() {
    // Reset to default values
    title = "Audio Playing"
    text = ""
  }

  override fun getNotificationId(): Int = notificationId

  override fun getChannelId(): String = CHANNEL_ID

  private fun buildNotification(): Notification {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")

    // Use a system icon that's guaranteed to exist
    val icon = android.R.drawable.ic_dialog_info

    val builder =
      NotificationCompat
        .Builder(context, CHANNEL_ID)
        .setContentTitle(title)
        .setContentText(text)
        .setSmallIcon(icon)
        .setPriority(NotificationCompat.PRIORITY_HIGH)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(false)
        .setAutoCancel(false)

    // Add content intent to open the app when notification is tapped
    val packageName = context.packageName
    val openAppIntent = context.packageManager?.getLaunchIntentForPackage(packageName)
    if (openAppIntent != null) {
      val pendingIntent =
        PendingIntent.getActivity(
          context,
          0,
          openAppIntent,
          PendingIntent.FLAG_IMMUTABLE,
        )
      builder.setContentIntent(pendingIntent)
    }

    return builder.build()
  }

  private fun createNotificationChannel() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val context = reactContext.get() ?: return
      val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

      val channel =
        NotificationChannel(
          CHANNEL_ID,
          CHANNEL_NAME,
          NotificationManager.IMPORTANCE_HIGH,
        ).apply {
          description = "Simple notification channel for audio playback"
          setShowBadge(true)
          lockscreenVisibility = Notification.VISIBILITY_PUBLIC
          enableLights(true)
          enableVibration(false)
        }

      notificationManager.createNotificationChannel(channel)
    }
  }
}

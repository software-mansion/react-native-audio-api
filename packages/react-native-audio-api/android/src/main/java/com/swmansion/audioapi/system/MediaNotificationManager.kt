package com.swmansion.audioapi.system

import android.annotation.SuppressLint
import android.app.Notification
import android.app.PendingIntent
import android.content.Intent
import android.content.res.Resources
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import com.facebook.react.bridge.ReactApplicationContext

class MediaNotificationManager(
  val reactContext: ReactApplicationContext,
  val notificationId: Int,
) {
  private var smallIcon: Int = 0
  private var customIcon: Int = 0

  private val play: NotificationCompat.Action? = null
  private var pause: NotificationCompat.Action? = null
  private var stop: NotificationCompat.Action? = null
  private var next: NotificationCompat.Action? = null
  private var previous: NotificationCompat.Action? = null
  private var skipForward: NotificationCompat.Action? = null
  private var skipBackward: NotificationCompat.Action? = null

  companion object {
    private const val REMOVE_NOTIFICATION: String = "audio_manager_remove_notification"
    private const val PACKAGE_NAME: String = "com.swmansion.audioapi.system"
  }

  init {
    val r: Resources = reactContext.resources
    val packageName: String = reactContext.packageName
    // Optional custom icon with fallback to the play icon
    smallIcon = r.getIdentifier("music_control_icon", "drawable", packageName)
    if (smallIcon == 0) smallIcon = r.getIdentifier("play", "drawable", packageName)
  }

  @Synchronized
  fun setCustomNotificationIcon(resourceName: String?) {
    if (resourceName == null) {
      customIcon = 0
      return
    }

    val r: Resources = reactContext.resources
    val packageName: String = reactContext.packageName

    customIcon = r.getIdentifier(resourceName, "drawable", packageName)
  }

  @SuppressLint("RestrictedApi")
  @Synchronized
  fun prepareNotification(
    builder: NotificationCompat.Builder,
    isPlaying: Boolean,
  ): Notification {
    // Add the buttons

    builder.mActions.clear()
    if (previous != null) builder.addAction(previous)
    if (skipBackward != null) builder.addAction(skipBackward)
    if (play != null && !isPlaying) builder.addAction(play)
    if (pause != null && isPlaying) builder.addAction(pause)
    if (stop != null) builder.addAction(stop)
    if (next != null) builder.addAction(next)
    if (skipForward != null) builder.addAction(skipForward)

    builder.setSmallIcon(if (customIcon != 0) customIcon else smallIcon)

    // Open the app when the notification is clicked
    val packageName: String = reactContext.packageName
    val openApp: Intent? = reactContext.packageManager.getLaunchIntentForPackage(packageName)
    try {
      builder.setContentIntent(
        PendingIntent.getActivity(
          reactContext,
          0,
          openApp,
          PendingIntent.FLAG_IMMUTABLE,
        ),
      )
    } catch (e: Exception) {
      println(e.message)
    }

    // Remove notification
    val remove = Intent(REMOVE_NOTIFICATION)
    remove.putExtra(PACKAGE_NAME, reactContext.applicationInfo.packageName)
    builder.setDeleteIntent(
      PendingIntent.getBroadcast(
        reactContext,
        0,
        remove,
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
      ),
    )

    return builder.build()
  }

  @SuppressLint("MissingPermission")
  @Synchronized
  fun show(
    builder: NotificationCompat.Builder?,
    isPlaying: Boolean,
  ) {
    NotificationManagerCompat.from(reactContext).notify(
      notificationId,
      prepareNotification(
        builder!!,
        isPlaying,
      ),
    )
  }

  fun hide() {
    NotificationManagerCompat.from(reactContext).cancel(notificationId)

    try {
//      val myIntent: Intent = Intent(
//        context,
//        MusicControlNotification.NotificationService::class.java
//      )
//      context.stopService(myIntent)
    } catch (e: java.lang.Exception) {
      println(e.message)
    }
  }
}

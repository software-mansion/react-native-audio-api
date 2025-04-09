package com.swmansion.audioapi.system

import android.annotation.SuppressLint
import android.app.Notification
import android.app.PendingIntent
import android.content.Intent
import android.content.res.Resources
import android.provider.ContactsContract
import android.support.v4.media.session.PlaybackStateCompat
import android.view.KeyEvent
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import com.facebook.react.bridge.ReactApplicationContext

class MediaNotificationManager(
  val reactContext: ReactApplicationContext,
  val notificationId: Int,
) {
  private var smallIcon: Int = 0
  private var customIcon: Int = 0

  private var play: NotificationCompat.Action? = null
  private var pause: NotificationCompat.Action? = null
  private var stop: NotificationCompat.Action? = null
  private var next: NotificationCompat.Action? = null
  private var previous: NotificationCompat.Action? = null
  private var skipForward: NotificationCompat.Action? = null
  private var skipBackward: NotificationCompat.Action? = null

  companion object {
    private const val REMOVE_NOTIFICATION: String = "audio_manager_remove_notification"
    private const val PACKAGE_NAME: String = "com.swmansion.audioapi.system"
    private const val MEDIA_BUTTON: String = "audio_manager_media_button"
  }

  init {
    val r: Resources = reactContext.resources
    val packageName: String = reactContext.packageName
    // Optional custom icon with fallback to the play icon
    smallIcon = r.getIdentifier("music_control_icon", "drawable", packageName)
    if (smallIcon == 0) smallIcon = r.getIdentifier("play", "drawable", packageName)
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

  @Synchronized
  fun updateActions(mask: Long) {
    play = createAction("play", "Play", mask, PlaybackStateCompat.ACTION_PLAY, play)
    pause = createAction("pause", "Pause", mask, PlaybackStateCompat.ACTION_PAUSE, pause)
    stop = createAction("stop", "Stop", mask, PlaybackStateCompat.ACTION_STOP, stop)
    next = createAction("next", "Next", mask, PlaybackStateCompat.ACTION_SKIP_TO_NEXT, next)
    previous =
      createAction(
        "previous",
        "Previous",
        mask,
        PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS,
        previous,
      )

    skipForward =
      createAction(
        "skip_forward_5",
        "Skip Forward",
        mask,
        PlaybackStateCompat.ACTION_FAST_FORWARD,
        skipForward,
      )

    skipBackward =
      createAction(
        "skip_backward_5",
        "Skip Backward",
        mask,
        PlaybackStateCompat.ACTION_REWIND,
        skipBackward,
      )
  }

  private fun createAction(
    iconName: String,
    title: String,
    mask: Long,
    action: Long,
    oldAction: NotificationCompat.Action?,
  ): NotificationCompat.Action? {
    if ((mask and action) == 0L) return null // When this action is not enabled, return null

    if (oldAction != null) return oldAction // If this action was already created, we won't create another instance

    // Finds the icon with the given name
    val r: Resources = reactContext.resources
    val packageName: String = reactContext.packageName
    val icon = r.getIdentifier(iconName, "drawable", packageName)

    // Creates the intent based on the action
    val keyCode = PlaybackStateCompat.toKeyCode(action)
    val intent = Intent(MEDIA_BUTTON)
    intent.putExtra(Intent.EXTRA_KEY_EVENT, KeyEvent(KeyEvent.ACTION_DOWN, keyCode))
    intent.putExtra(ContactsContract.Directory.PACKAGE_NAME, packageName)
    val i =
      PendingIntent.getBroadcast(
        reactContext,
        keyCode,
        intent,
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
      )

    return NotificationCompat.Action(icon, title, i)
  }
}

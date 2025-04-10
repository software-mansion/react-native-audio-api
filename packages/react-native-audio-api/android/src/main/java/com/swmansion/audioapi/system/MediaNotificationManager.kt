package com.swmansion.audioapi.system

import android.annotation.SuppressLint
import android.app.Notification
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.content.res.Resources
import android.os.Binder
import android.os.Build
import android.os.IBinder
import android.provider.ContactsContract
import android.support.v4.media.session.PlaybackStateCompat
import android.view.KeyEvent
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.swmansion.audioapi.R
import java.lang.ref.WeakReference

class MediaNotificationManager(
  val reactContext: ReactApplicationContext,
  val notificationId: Int,
  val channelId: String
) {
  private var smallIcon: Int = R.drawable.play
  private var customIcon: Int = 0

  private var play: NotificationCompat.Action? = null
  private var pause: NotificationCompat.Action? = null
  private var stop: NotificationCompat.Action? = null
  private var next: NotificationCompat.Action? = null
  private var previous: NotificationCompat.Action? = null
  private var skipForward: NotificationCompat.Action? = null
  private var skipBackward: NotificationCompat.Action? = null

  companion object {
    const val REMOVE_NOTIFICATION: String = "audio_manager_remove_notification"
    const val PACKAGE_NAME: String = "com.swmansion.audioapi.system"
    const val MEDIA_BUTTON: String = "audio_manager_media_button"
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
      val myIntent = Intent(
        reactContext,
        NotificationService::class.java
      )
      reactContext.stopService(myIntent)
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

  inner class NotificationService : Service() {
    private val binder = LocalBinder()
    private var notification: Notification? = null

    inner class LocalBinder : Binder() {
      private var weakService: WeakReference<NotificationService>? = null

      fun onBind(service: NotificationService) {
        weakService = WeakReference(service)
      }

      fun getService(): NotificationService? = weakService?.get()
    }

    override fun onBind(intent: Intent): IBinder {
      binder.onBind(this)
      return binder
    }

    fun forceForeground() {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        val intent = Intent(this, NotificationService::class.java)
        ContextCompat.startForegroundService(this, intent)
        notification = MediaNotificationManager(reactContext, notificationId, channelId)
          .prepareNotification(NotificationCompat.Builder(this, channelId), false)
        startForeground(notificationId, notification)
      }
    }

    override fun onCreate() {
      super.onCreate()
      try {
        notification = MediaNotificationManager(reactContext, notificationId, channelId)
          .prepareNotification(NotificationCompat.Builder(this, channelId), false)
        startForeground(notificationId, notification)
      } catch (ex: Exception) {
        ex.printStackTrace()
      }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
      onCreate() // reuse the onCreate logic for service re/starting
      return START_NOT_STICKY
    }

    override fun onTaskRemoved(rootIntent: Intent?) {
      super.onTaskRemoved(rootIntent)
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        stopForeground(STOP_FOREGROUND_REMOVE)
      }
      stopSelf() // ensure service is stopped if the task is removed
    }

    override fun onDestroy() {
      super.onDestroy()

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        stopForeground(STOP_FOREGROUND_REMOVE)
      }

      stopSelf() // ensure service is stopped on destroy
    }
  }
}

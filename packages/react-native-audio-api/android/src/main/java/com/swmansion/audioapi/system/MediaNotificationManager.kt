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
import android.util.Log
import android.view.KeyEvent
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.swmansion.audioapi.R
import java.lang.ref.WeakReference

class MediaNotificationManager(
  private val reactContext: WeakReference<ReactApplicationContext>,
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
    builder.mActions.clear()

    if (previous != null) {
      builder.addAction(previous)
    }

    if (skipBackward != null) {
      builder.addAction(skipBackward)
    }

    if (play != null && !isPlaying) {
      builder.addAction(play)
    }

    if (pause != null && isPlaying) {
      builder.addAction(pause)
    }

    if (stop != null) {
      builder.addAction(stop)
    }

    if (next != null) {
      builder.addAction(next)
    }

    if (skipForward != null) {
      builder.addAction(skipForward)
    }

    builder.setSmallIcon(if (customIcon != 0) customIcon else smallIcon)

    val packageName: String? = reactContext.get()?.packageName
    val openApp: Intent? = reactContext.get()?.packageManager?.getLaunchIntentForPackage(packageName!!)
    try {
      builder.setContentIntent(
        PendingIntent.getActivity(
          reactContext.get(),
          0,
          openApp,
          PendingIntent.FLAG_IMMUTABLE,
        ),
      )
    } catch (e: Exception) {
      Log.w("AudioManagerModule", "Error creating content intent: ${e.message}")
    }

    val remove = Intent(REMOVE_NOTIFICATION)
    remove.putExtra(PACKAGE_NAME, reactContext.get()?.applicationInfo?.packageName)
    builder.setDeleteIntent(
      PendingIntent.getBroadcast(
        reactContext.get(),
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
    NotificationManagerCompat.from(reactContext.get()!!).notify(
      MediaSessionManager.NOTIFICATION_ID,
      prepareNotification(
        builder!!,
        isPlaying,
      ),
    )
  }

  fun hide() {
    NotificationManagerCompat.from(reactContext.get()!!).cancel(MediaSessionManager.NOTIFICATION_ID)

    try {
      val myIntent =
        Intent(
          reactContext.get(),
          NotificationService::class.java,
        )
      reactContext.get()?.stopService(myIntent)
    } catch (e: java.lang.Exception) {
      Log.w("AudioManagerModule", "Error stopping service: ${e.message}")
    }
  }

  @Synchronized
  fun updateActions(mask: Long) {
    play = createAction("play", "Play", mask, PlaybackStateCompat.ACTION_PLAY, play)
    pause = createAction("pause", "Pause", mask, PlaybackStateCompat.ACTION_PAUSE, pause)
    stop = createAction("stop", "Stop", mask, PlaybackStateCompat.ACTION_STOP, stop)
    next = createAction("next", "Next", mask, PlaybackStateCompat.ACTION_SKIP_TO_NEXT, next)
    previous = createAction("previous", "Previous", mask, PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS, previous)
    skipForward = createAction("skip_forward_15", "Skip Forward", mask, PlaybackStateCompat.ACTION_FAST_FORWARD, skipForward)
    skipBackward = createAction("skip_backward_15", "Skip Backward", mask, PlaybackStateCompat.ACTION_REWIND, skipBackward)
  }

  private fun createAction(
    iconName: String,
    title: String,
    mask: Long,
    action: Long,
    oldAction: NotificationCompat.Action?,
  ): NotificationCompat.Action? {
    if ((mask and action) == 0L) {
      return null
    }

    if (oldAction != null) {
      return oldAction
    }

    val r: Resources? = reactContext.get()?.resources
    val packageName: String? = reactContext.get()?.packageName
    val icon = r?.getIdentifier(iconName, "drawable", packageName)

    val keyCode = PlaybackStateCompat.toKeyCode(action)
    val intent = Intent(MEDIA_BUTTON)
    intent.putExtra(Intent.EXTRA_KEY_EVENT, KeyEvent(KeyEvent.ACTION_DOWN, keyCode))
    intent.putExtra(ContactsContract.Directory.PACKAGE_NAME, packageName)
    val i =
      PendingIntent.getBroadcast(
        reactContext.get(),
        keyCode,
        intent,
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
      )

    return NotificationCompat.Action(icon!!, title, i)
  }

  class NotificationService : Service() {
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
        notification =
          MediaSessionManager.mediaNotificationManager
            .prepareNotification(NotificationCompat.Builder(this, MediaSessionManager.CHANNEL_ID), false)
        startForeground(MediaSessionManager.NOTIFICATION_ID, notification)
      }
    }

    override fun onCreate() {
      super.onCreate()
      try {
        notification =
          MediaSessionManager.mediaNotificationManager
            .prepareNotification(NotificationCompat.Builder(this, MediaSessionManager.CHANNEL_ID), false)
        startForeground(MediaSessionManager.NOTIFICATION_ID, notification)
      } catch (ex: Exception) {
        Log.w("AudioManagerModule", "Error starting service: ${ex.message}")
      }
    }

    override fun onStartCommand(
      intent: Intent?,
      flags: Int,
      startId: Int,
    ): Int {
      onCreate()
      return START_NOT_STICKY
    }

    override fun onTaskRemoved(rootIntent: Intent?) {
      super.onTaskRemoved(rootIntent)
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        stopForeground(STOP_FOREGROUND_REMOVE)
      }
      stopSelf()
    }

    override fun onDestroy() {
      super.onDestroy()

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        stopForeground(STOP_FOREGROUND_REMOVE)
      }

      stopSelf()
    }
  }
}

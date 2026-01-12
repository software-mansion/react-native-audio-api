package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.PendingIntent
import android.content.Context
import android.content.Context.RECEIVER_NOT_EXPORTED
import android.content.Intent
import android.content.IntentFilter
import android.graphics.Bitmap
import android.graphics.drawable.Icon
import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import androidx.core.graphics.drawable.IconCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.R
import java.lang.ref.WeakReference

class RecordingNotification(
  private val reactContext: WeakReference<ReactApplicationContext>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
  private val notificationId: Int,
  private val channelId: String,
) : BaseNotification {
  companion object {
    private const val TAG = "RecordingNotification"
    const val ID = 200
  }

  private var builder: NotificationCompat.Builder? = null
  private var receiver: RecordingNotificationReceiver? = null
  private var initialized: Boolean = false

  private var pauseIntent: Intent? = null
  private var resumeIntent: Intent? = null
  private var title: String? = null
  private var contentText: String? = null
  private var paused: Boolean = true
  private var smallIconResourceName: String? = null
  private var largeIconResourceName: String? = null
  private var pauseIconResourceName: String? = null
  private var resumeIconResourceName: String? = null
  private var backgroundColor: Int? = null

  @RequiresApi(Build.VERSION_CODES.O)
  override fun show(options: ReadableMap?): Notification {
    initialize()
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    parseMapFromRN(options)
    val builder = getBuilder()
    builder.clearActions()

    if (smallIconResourceName != null) {
      builder.setSmallIcon(context.resources.getIdentifier(smallIconResourceName, "drawable", context.packageName))
    }

    if (largeIconResourceName != null) {
      val icon = Icon.createWithResource(context, context.resources.getIdentifier(largeIconResourceName, "drawable", context.packageName))
      builder.setLargeIcon(icon)
    }

    if (backgroundColor != null) {
      builder.setColor(backgroundColor!!)
    }

    val pauseResumeIntent =
      if (paused) {
        resumeIntent!!
      } else {
        pauseIntent!!
      }

    val pauseResumePendingIntent =
      PendingIntent.getBroadcast(
        context,
        0,
        pauseResumeIntent,
        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
      )

    val description = if (paused) "Resume" else "Stop"
    val pauseId =
      if (pauseIconResourceName != null) {
        context.resources.getIdentifier(pauseIconResourceName, "drawable", context.packageName)
      } else {
        android.R.drawable.ic_media_pause
      }
    val resumeId =
      if (resumeIconResourceName != null) {
        context.resources.getIdentifier(resumeIconResourceName, "drawable", context.packageName)
      } else {
        android.R.drawable.ic_media_play
      }

    val icon = IconCompat.createWithResource(context, if (paused) resumeId else pauseId)

    val action =
      NotificationCompat.Action
        .Builder(
          icon,
          description,
          pauseResumePendingIntent,
        ).build()

//    collapsedView.setTextViewText(R.id.notification_title, this.title)
//    collapsedView.setTextViewText(R.id.notification_content, this.contentText)
//    collapsedView.setImageViewResource(R.id.notification_icon_small, if (paused) resumeId else pauseId)
//    collapsedView.setOnClickPendingIntent(R.id.notification_icon_small, pauseResumePendingIntent)
//
//    // 5. Update Expanded View
//    expandedView.setTextViewText(R.id.expanded_title, this.title)
//    expandedView.setTextViewText(R.id.expanded_body, this.contentText)
//    expandedView.setImageViewResource(R.id.notification_icon_large, if (paused) resumeId else pauseId)
//    expandedView.setOnClickPendingIntent(R.id.notification_icon_large, pauseResumePendingIntent)

    builder
//      .setCustomContentView(collapsedView)
//      .setCustomBigContentView(expandedView)
      .setContentTitle(this.title)
      .setContentText(this.contentText)
      .addAction(action)

    if (this.backgroundColor != null) {
      builder.setColor(this.backgroundColor!!)
    }

    return builder.build()
  }

  private fun loadBitmapFromUri(
    context: Context,
    uriString: String?,
  ): Bitmap? =
    // not used currently, left for future reference
    null
//    try {
//      val uri = android.net.Uri.parse(uriString)
//      val inputStream: InputStream
//      if (uri.scheme == "http" || uri.scheme == "https") {
//        // web URL
//        val connection = java.net.URL(uriString).openConnection()
//        connection.doInput = true
//        connection.connect()
//        inputStream = connection.inputStream
//      } else {
//        // local files
//        inputStream = context.contentResolver.openInputStream(uri)!!
//      }
//      android.graphics.BitmapFactory.decodeStream(inputStream)
//    } catch (e: Exception) {
//      Log.e(TAG, "Failed to load bitmap from URI: $uriString", e)
//      null
//    }

  private fun getBuilder(): NotificationCompat.Builder {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (builder == null) {
      val openAppIntent = context.packageManager.getLaunchIntentForPackage(context.packageName)
      val pendingIntent = PendingIntent.getActivity(context, 0, openAppIntent, PendingIntent.FLAG_IMMUTABLE)

      val style =
        androidx.media.app.NotificationCompat
          .MediaStyle()
          .setShowActionsInCompactView(0)

      builder =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
          NotificationCompat
            .Builder(context, channelId)
            .setOngoing(true)
            .setContentIntent(pendingIntent)
            .setStyle(style)
        } else {
          throw IllegalStateException("RecordingNotification requires Android O or higher")
        }
      if (smallIconResourceName == null) {
        builder!!.setSmallIcon(android.R.drawable.ic_btn_speak_now)
      }
    }
    return builder!!
  }

  @RequiresApi(Build.VERSION_CODES.O)
  private fun initialize() {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (!initialized) {
      createNotificationChannel(context)
      receiver =
        RecordingNotificationReceiver(audioAPIModule.get()!!)
      val filter =
        IntentFilter().apply {
          addAction(RecordingNotificationReceiver.NOTIFICATION_RECORDING_STOPPED)
          addAction(RecordingNotificationReceiver.NOTIFICATION_RECORDING_RESUMED)
        }
      ContextCompat.registerReceiver(
        context,
        receiver,
        filter,
        ContextCompat.RECEIVER_NOT_EXPORTED,
      )

      pauseIntent =
        Intent(RecordingNotificationReceiver.NOTIFICATION_RECORDING_STOPPED).apply {
          `package` = context.packageName
        }

      resumeIntent =
        Intent(RecordingNotificationReceiver.NOTIFICATION_RECORDING_RESUMED).apply {
          `package` = context.packageName
        }
      initialized = true
    }
  }

  private fun createNotificationChannel(context: ReactApplicationContext) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val channel =
        android.app
          .NotificationChannel(
            channelId,
            "Recording Audio",
            android.app.NotificationManager.IMPORTANCE_LOW,
          ).apply {
            description = "Notifications for ongoing audio recordings"
            lockscreenVisibility = Notification.VISIBILITY_PUBLIC
          }
      val notificationManager =
        context.getSystemService(Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
      notificationManager.createNotificationChannel(channel)
    }
    Log.d(TAG, "Notification channel created: $channelId")
  }

  private fun parseMapFromRN(options: ReadableMap?) {
    this.title = if (options?.hasKey("title") == true) options.getString("title") else "Recording Audio"
    this.contentText =
      if (options?.hasKey("contentText") == true) {
        options.getString("contentText")
      } else {
        "Audio recording is in progress/paused"
      }
    this.smallIconResourceName = if (options?.hasKey("smallIconResourceName") == true) options.getString("smallIconResourceName") else null
    this.largeIconResourceName = if (options?.hasKey("largeIconResourceName") == true) options.getString("largeIconResourceName") else null
    this.pauseIconResourceName = if (options?.hasKey("pauseIconResourceName") == true) options.getString("pauseIconResourceName") else null
    this.resumeIconResourceName =
      if (options?.hasKey("resumeIconResourceName") == true) options.getString("resumeIconResourceName") else null
    this.backgroundColor = if (options?.hasKey("color") == true) options.getInt("color") else null
    this.paused = if (options?.hasKey("paused") == true) options.getBoolean("paused") else false
  }

  override fun hide() {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (receiver != null) {
      context.unregisterReceiver(receiver)
      receiver = null
    }
  }

  override fun getNotificationId(): Int = notificationId

  override fun getChannelId(): String = channelId
}

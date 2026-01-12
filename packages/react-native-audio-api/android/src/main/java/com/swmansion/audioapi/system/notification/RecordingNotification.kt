package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.Bitmap
import android.graphics.Color
import android.graphics.drawable.Icon
import android.os.Build
import android.util.Log
import androidx.core.app.NotificationCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.system.notification.RecordingNotificationReceiver.Companion.setAudioAPIModule
import java.io.InputStream
import java.lang.ref.WeakReference

/**
 * RecordingNotification
 *
 * Simple notification for audio recording:
 * - Shows recording status with red background when recording
 * - Simple start/stop button with microphone icon
 * - Is persistent and cannot be swiped away when recording
 * - Notifies its dismissal via RecordingNotificationReceiver
 */
class RecordingNotification(
  private val reactContext: WeakReference<ReactApplicationContext>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
  private val notificationId: Int,
  private val channelId: String,
) : BaseNotification {
  companion object {
    private const val TAG = "RecordingNotification"
    const val ACTION_START = "com.swmansion.audioapi.RECORDING_START"
    const val ACTION_STOP = "com.swmansion.audioapi.RECORDING_STOP"
  }

  private var notificationBuilder: NotificationCompat.Builder? = null
  private var isRecording: Boolean = false
  private var title: String = "Audio Recording"
  private var description: String = "Ready to record"
  private var receiver: RecordingNotificationReceiver? = null
  private var startEnabled: Boolean = true
  private var stopEnabled: Boolean = true

  private var pauseIntent: Intent? = null
  private var resumeIntent: Intent? = null
  private var title: String? = null
  private var contentText: String? = null
  private var paused: Boolean = true
  private var smallIconResourceName: String? = null
  private var backgroundColor: Int? = null
  private var largeIconUri: String? = null
  private var largeIconThread: Thread? = null

  @RequiresApi(Build.VERSION_CODES.O)
  override fun show(options: ReadableMap?): Notification {
    initialize()
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    parseMapFromRN(options)
    val builder = getBuilder()
    builder.setSmallIcon(context.resources.getIdentifier(smallIconResourceName, "drawable", context.packageName))
    builder.setColor(Color.RED)
    if (largeIconUri != null) {
      largeIconThread?.interrupt()
      largeIconThread =
        Thread {
          val bitmap = loadBitmapFromUri(context, largeIconUri)
          if (bitmap != null) {
            context.runOnUiQueueThread {
              try {
                builder.setLargeIcon(bitmap)
                val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
                notificationManager.notify(notificationId, builder.build())
              } catch (e: Exception) {
                Log.e(TAG, "Failed to update notification with large icon", e)
              }
            }
            builder.setLargeIcon(bitmap)
          }
          largeIconThread = null
        }
      largeIconThread?.start()
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

    val icon = if (paused) android.R.drawable.ic_media_play else android.R.drawable.ic_media_pause

    builder.clearActions()
    val action =
      NotificationCompat.Action
        .Builder(
          icon,
          description,
          pauseResumePendingIntent,
        ).build()

    builder
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
    try {
      val uri = android.net.Uri.parse(uriString)
      val inputStream: InputStream
      if (uri.scheme == "http" || uri.scheme == "https") {
        // web URL
        val connection = java.net.URL(uriString).openConnection()
        connection.doInput = true
        connection.connect()
        inputStream = connection.inputStream
      } else {
        // local files
        inputStream = context.contentResolver.openInputStream(uri)!!
      }
      android.graphics.BitmapFactory.decodeStream(inputStream)
    } catch (e: Exception) {
      Log.e(TAG, "Failed to load bitmap from URI: $uriString", e)
      null
    }

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
      if (largeIconUri == null) {
        builder!!.setLargeIcon(
          Icon.createWithResource(context, android.R.drawable.ic_btn_speak_now),
        )
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
        RecordingNotificationReceiver(this).apply {
          setAudioAPIModule(audioAPIModule.get())
        }
      val filter1 =
        IntentFilter(RecordingNotificationReceiver.NOTIFICATION_RECORDING_STOPPED)
      val filter2 = IntentFilter(RecordingNotificationReceiver.NOTIFICATION_RECORDING_RESUMED)
      context.registerReceiver(receiver, filter1, RECEIVER_NOT_EXPORTED)
      context.registerReceiver(receiver, filter2, RECEIVER_NOT_EXPORTED)
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
        context.getSystemService(android.content.Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
      notificationManager.createNotificationChannel(channel)
    }
    Log.d(TAG, "Notification channel created: $channelId")
  }

  private fun parseMapFromRN(options: ReadableMap?) {
    this.title = if (options?.hasKey("title") == true) options.getString("title") else "Recording Audio"
    this.contentText =
      if (options?.hasKey("contentText") ==
        true
      ) {
        options.getString("contentText")
      } else {
        "Audio recording is in progress/paused"
      }
    this.smallIconResourceName = if (options?.hasKey("smallIconResourceName") == true) options.getString("smallIconResourceName") else null
    this.largeIconUri = if (options?.hasKey("largeIcon") == true) options.getString("largeIcon") else null
    this.backgroundColor =
      if (options?.hasKey("color") == true) options.getInt("color") else null

    this.paused = if (options?.hasKey("paused") == true) options.getBoolean("paused") else false
  }

  override fun hide() {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (receiver != null) {
      context.unregisterReceiver(receiver)
      receiver = null
    }

    return buildNotification()
  }

  override fun reset() {
    // Unregister receiver
    unregisterReceiver()

    // Reset state
    title = "Audio Recording"
    description = "Ready to record"
    isRecording = false
    notificationBuilder = null
  }

  override fun getNotificationId(): Int = notificationId

  override fun getChannelId(): String = channelId

  override fun update(options: ReadableMap?): Notification {
    if (options == null) {
      return buildNotification()
    }

    // Handle control enable/disable
    if (options.hasKey("control") && options.hasKey("enabled")) {
      val control = options.getString("control")
      val enabled = options.getBoolean("enabled")
      when (control) {
        "start" -> startEnabled = enabled
        "stop" -> stopEnabled = enabled
      }
      updateActions()
      return buildNotification()
    }

    // Update metadata
    if (options.hasKey("title")) {
      title = options.getString("title") ?: "Audio Recording"
    }

    if (options.hasKey("description")) {
      description = options.getString("description") ?: "Ready to record"
    }

    // Update recording state
    if (options.hasKey("state")) {
      when (options.getString("state")) {
        "recording" -> isRecording = true
        "stopped" -> isRecording = false
      }
    }

    // Update notification content
    val statusText =
      description.ifEmpty {
        if (isRecording) "Recording..." else "Ready to record"
      }
    notificationBuilder
      ?.setContentTitle(title)
      ?.setContentText(statusText)
      ?.setOngoing(isRecording)

    // Set red color when recording
    if (isRecording) {
      notificationBuilder
        ?.setColor(Color.RED)
        ?.setColorized(true)
    } else {
      notificationBuilder
        ?.setColorized(false)
    }

    // Update action button
    updateActions()

    return buildNotification()
  }

  private fun buildNotification(): Notification =
    notificationBuilder?.build()
      ?: throw IllegalStateException("Notification not initialized. Call init() first.")

  private fun updateActions() {
    val context = reactContext.get() ?: return

    // Clear existing actions
    notificationBuilder?.clearActions()

    // Add appropriate action based on recording state and enabled controls
    // Note: Android shows text labels in collapsed view, icons only in expanded/Auto/Wear
    if (isRecording && stopEnabled) {
      // Show STOP button when recording
      val stopIntent = Intent(ACTION_STOP)
      stopIntent.setPackage(context.packageName)
      val stopPendingIntent =
        PendingIntent.getBroadcast(
          context,
          1001,
          stopIntent,
          PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
      val stopAction =
        NotificationCompat.Action
          .Builder(
            android.R.drawable.ic_delete,
            "Stop",
            stopPendingIntent,
          ).build()
      notificationBuilder?.addAction(stopAction)
    } else if (!isRecording && startEnabled) {
      // Show START button when not recording
      val startIntent = Intent(ACTION_START)
      startIntent.setPackage(context.packageName)
      val startPendingIntent =
        PendingIntent.getBroadcast(
          context,
          1000,
          startIntent,
          PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
      val startAction =
        NotificationCompat.Action
          .Builder(
            android.R.drawable.ic_btn_speak_now,
            "Record",
            startPendingIntent,
          ).build()
      notificationBuilder?.addAction(startAction)
    }

    // Use BigTextStyle to ensure actions are visible
    val statusText =
      description.ifEmpty {
        if (isRecording) "Recording in progress..." else "Ready to record"
      }
    notificationBuilder?.setStyle(
      NotificationCompat
        .BigTextStyle()
        .bigText(statusText),
    )
  }

  private fun createNotificationChannel() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val context = reactContext.get() ?: return

      val channel =
        NotificationChannel(
          channelId,
          "Audio Recording",
          NotificationManager.IMPORTANCE_HIGH,
        ).apply {
          description = "Recording controls and status"
          setShowBadge(true)
          lockscreenVisibility = Notification.VISIBILITY_PUBLIC
          enableLights(true)
          lightColor = Color.RED
          enableVibration(false)
        }

      val notificationManager =
        context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
      notificationManager.createNotificationChannel(channel)

      Log.d(TAG, "Notification channel created: $channelId")
    }
  }

  private fun registerReceiver() {
    val context = reactContext.get() ?: return

    if (receiver == null) {
      receiver = RecordingNotificationReceiver()
      RecordingNotificationReceiver.setAudioAPIModule(audioAPIModule.get())

      val filter = IntentFilter()
      filter.addAction(ACTION_START)
      filter.addAction(ACTION_STOP)
      filter.addAction(RecordingNotificationReceiver.ACTION_NOTIFICATION_DISMISSED)

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        context.registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED)
      } else {
        context.registerReceiver(receiver, filter)
      }

      Log.d(TAG, "RecordingNotificationReceiver registered")
    }
  }

  private fun unregisterReceiver() {
    val context = reactContext.get() ?: return

    receiver?.let {
      try {
        context.unregisterReceiver(it)
        receiver = null
        Log.d(TAG, "RecordingNotificationReceiver unregistered")
      } catch (e: Exception) {
        Log.e(TAG, "Error unregistering receiver: ${e.message}", e)
      }
    }
  }
}

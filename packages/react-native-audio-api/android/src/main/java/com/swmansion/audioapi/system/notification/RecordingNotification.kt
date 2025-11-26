package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.Color
import android.os.Build
import android.util.Log
import androidx.core.app.NotificationCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.AudioAPIModule
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

  override fun init(params: ReadableMap?): Notification {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")

    // Register broadcast receiver
    registerReceiver()

    // Create notification channel first
    createNotificationChannel()

    // Create notification builder
    notificationBuilder =
      NotificationCompat
        .Builder(context, channelId)
        .setSmallIcon(android.R.drawable.ic_btn_speak_now)
        .setContentTitle(title)
        .setContentText(description)
        .setPriority(NotificationCompat.PRIORITY_HIGH)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(false)
        .setAutoCancel(false)

    // Set content intent to open app
    val packageName = context.packageName
    val openAppIntent = context.packageManager.getLaunchIntentForPackage(packageName)
    if (openAppIntent != null) {
      val pendingIntent =
        PendingIntent.getActivity(
          context,
          0,
          openAppIntent,
          PendingIntent.FLAG_IMMUTABLE,
        )
      notificationBuilder?.setContentIntent(pendingIntent)
    }

    // Set delete intent to handle dismissal
    val deleteIntent = Intent(RecordingNotificationReceiver.ACTION_NOTIFICATION_DISMISSED)
    deleteIntent.setPackage(context.packageName)
    val deletePendingIntent =
      PendingIntent.getBroadcast(
        context,
        notificationId,
        deleteIntent,
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
      )
    notificationBuilder?.setDeleteIntent(deletePendingIntent)

    // Apply initial params if provided
    if (params != null) {
      update(params)
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

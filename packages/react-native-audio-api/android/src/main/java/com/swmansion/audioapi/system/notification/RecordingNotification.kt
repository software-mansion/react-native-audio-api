package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.ComponentCallbacks
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.drawable.Icon
import android.os.Build
import android.util.Log
import android.widget.RemoteViews
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.R
import com.swmansion.audioapi.system.notification.state.RecordingNotificationState
import java.lang.ref.WeakReference

class RecordingNotification(
  private val reactContext: WeakReference<ReactApplicationContext>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
  private val notificationId: Int,
  private val channelId: String,
) : BaseNotification,
  ComponentCallbacks {
  companion object {
    private const val TAG = "RecordingNotification"
    const val ID = 200
  }

  private var state: RecordingNotificationState =
    RecordingNotificationState(
      darkTheme =
        reactContext
          .get()!!
          .resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK == Configuration.UI_MODE_NIGHT_YES,
      initialized = false,
    )

  private fun initializeNotification() {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (!state.initialized) {
      context.registerComponentCallbacks(this)
      createNotificationChannel(context)
      state.receiver =
        RecordingNotificationReceiver(audioAPIModule.get()!!)
      val filter =
        IntentFilter().apply {
          addAction(RecordingNotificationReceiver.NOTIFICATION_RECORDING_STOPPED)
          addAction(RecordingNotificationReceiver.NOTIFICATION_RECORDING_RESUMED)
        }
      ContextCompat.registerReceiver(
        context,
        state.receiver,
        filter,
        ContextCompat.RECEIVER_NOT_EXPORTED,
      )

      state.pauseIntent =
        Intent(RecordingNotificationReceiver.NOTIFICATION_RECORDING_STOPPED).apply {
          `package` = context.packageName
        }

      state.resumeIntent =
        Intent(RecordingNotificationReceiver.NOTIFICATION_RECORDING_RESUMED).apply {
          `package` = context.packageName
        }
      state.darkTheme = context.resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK == Configuration.UI_MODE_NIGHT_YES
      state.initialized = true
    }
  }

  override fun show(options: ReadableMap?): Notification {
    initializeNotification()
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (options != state.cachedRNOptions) {
      state.cachedRNOptions = options
      parseMapFromRN(options)
    }
    val builder = getBuilder()

    if (state.smallIconResourceName != null) {
      builder.setSmallIcon(context.resources.getIdentifier(state.smallIconResourceName, "drawable", context.packageName))
    }

    if (state.largeIconResourceName != null) {
      val icon =
        Icon.createWithResource(
          context,
          context.resources.getIdentifier(state.largeIconResourceName, "drawable", context.packageName),
        )
      builder.setLargeIcon(icon)
    }

    if (state.backgroundColor != null) {
      builder.setColor(state.backgroundColor!!)
    }

    val collapsedView = RemoteViews(context.packageName, R.layout.notification_collapsed)
    val expandedView = RemoteViews(context.packageName, R.layout.notification_expanded)

    val (pauseResumePendingIntent, iconId) = setupPauseResumeIntent(context)

    setupRemoteView(listOf(collapsedView, expandedView), pauseResumePendingIntent, iconId)

    builder
      .setStyle(NotificationCompat.DecoratedCustomViewStyle())
      .setCustomContentView(collapsedView)
      .setCustomBigContentView(expandedView)
      .setContentTitle(state.title)
      .setContentText(state.contentText)

    if (state.backgroundColor != null) {
      builder.setColor(state.backgroundColor!!)
    }

    return builder.build()
  }

  private fun setupPauseResumeIntent(context: Context): Pair<PendingIntent, Int> {
    val pauseResumeIntent =
      if (state.paused) {
        state.resumeIntent
      } else {
        state.pauseIntent
      }

    val pauseResumePendingIntent =
      PendingIntent.getBroadcast(
        context,
        0,
        pauseResumeIntent!!,
        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
      )

    val pauseId =
      if (state.pauseIconResourceName != null) {
        context.resources.getIdentifier(state.pauseIconResourceName, "drawable", context.packageName)
      } else {
        android.R.drawable.ic_media_pause
      }
    val resumeId =
      if (state.resumeIconResourceName != null) {
        context.resources.getIdentifier(state.resumeIconResourceName, "drawable", context.packageName)
      } else {
        android.R.drawable.ic_media_play
      }

    val iconId = if (state.paused) resumeId else pauseId
    return pauseResumePendingIntent to iconId
  }

  private fun setupRemoteView(
    views: List<RemoteViews>,
    pauseResumePendingIntent: PendingIntent,
    iconId: Int,
  ) {
    val iconColor =
      if (state.darkTheme) {
        Color.WHITE // Dark Mode -> White Icon
      } else {
        Color.BLACK // Light Mode -> Black Icon
      }
    for (view in views) {
      view.setTextViewText(R.id.notification_title, state.title)
      view.setTextViewText(R.id.notification_content, state.contentText)
      view.setImageViewResource(R.id.notification_action_btn, iconId)
      view.setInt(R.id.notification_action_btn, "setColorFilter", iconColor)
      view.setOnClickPendingIntent(R.id.notification_action_btn, pauseResumePendingIntent)
    }
  }

// not used currently, left for future reference
//  private fun loadBitmapFromUri(
//    context: Context,
//    uriString: String?,
//  ): Bitmap? =
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
    if (state.builder == null) {
      val openAppIntent = context.packageManager.getLaunchIntentForPackage(context.packageName)
      val pendingIntent = PendingIntent.getActivity(context, 0, openAppIntent, PendingIntent.FLAG_IMMUTABLE)

      state.builder =
        NotificationCompat
          .Builder(context, channelId)
          .setOngoing(true)
          .setContentIntent(pendingIntent)
    }
    if (state.smallIconResourceName == null) {
      state.builder!!.setSmallIcon(android.R.drawable.ic_btn_speak_now)
    }
    return state.builder!!
  }

  private fun createNotificationChannel(context: ReactApplicationContext) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val channel =
        NotificationChannel(
          channelId,
          "Recording Audio",
          NotificationManager.IMPORTANCE_LOW,
        ).apply {
          description = "Notifications for ongoing audio recordings"
          lockscreenVisibility = Notification.VISIBILITY_PUBLIC
        }
      val notificationManager =
        context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
      notificationManager.createNotificationChannel(channel)
    }
    Log.d(TAG, "Notification channel created: $channelId")
  }

  private fun parseMapFromRN(options: ReadableMap?) {
    state.title = if (options?.hasKey("title") == true) options.getString("title") else state.title ?: "Recording Audio"
    state.contentText =
      if (options?.hasKey("contentText") == true) {
        options.getString("contentText")
      } else {
        state.contentText ?: "Audio recording is in progress/paused"
      }
    state.smallIconResourceName =
      if (options?.hasKey("smallIconResourceName") ==
        true
      ) {
        options.getString("smallIconResourceName")
      } else {
        state.smallIconResourceName ?: null
      }
    state.largeIconResourceName =
      if (options?.hasKey("largeIconResourceName") ==
        true
      ) {
        options.getString("largeIconResourceName")
      } else {
        state.largeIconResourceName ?: null
      }
    state.pauseIconResourceName =
      if (options?.hasKey("pauseIconResourceName") ==
        true
      ) {
        options.getString("pauseIconResourceName")
      } else {
        state.pauseIconResourceName ?: null
      }
    state.resumeIconResourceName =
      if (options?.hasKey("resumeIconResourceName") ==
        true
      ) {
        options.getString("resumeIconResourceName")
      } else {
        state.resumeIconResourceName ?: null
      }
    state.backgroundColor = if (options?.hasKey("color") == true) options.getInt("color") else state.backgroundColor ?: null
    state.paused = if (options?.hasKey("paused") == true) options.getBoolean("paused") else false
  }

  override fun hide() {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (state.receiver != null) {
      context.unregisterReceiver(state.receiver)
      context.unregisterComponentCallbacks(this)
      state.receiver = null
    }
    val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    notificationManager.cancel(notificationId)
    state.initialized = false
    state.builder = null
  }

  override fun getNotificationId(): Int = notificationId

  override fun getChannelId(): String = channelId

  @RequiresApi(Build.VERSION_CODES.O)
  override fun onConfigurationChanged(newConfig: Configuration) {
    val currentNightMode = newConfig.uiMode and Configuration.UI_MODE_NIGHT_MASK == Configuration.UI_MODE_NIGHT_YES
    if (currentNightMode != state.darkTheme) {
      // Theme changed, rebuild notification
      state.darkTheme = currentNightMode
      val notification = show(state.cachedRNOptions)
      val context = reactContext.get()
      if (context != null) {
        val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(notificationId, notification)
      }
    }
  }

  @Deprecated("Deprecated in Java")
  override fun onLowMemory() {
    // left to listen for ui mode changes
  }
}

package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.drawable.Icon
import android.net.Uri
import android.os.Build
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.system.notification.state.RecordingNotificationState
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

    private const val REQUEST_CODE_CONTENT = 2000
    private const val REQUEST_CODE_PAUSE = 2001
    private const val REQUEST_CODE_RESUME = 2002
    private const val REQUEST_CODE_STOP = 2003
  }

  private val state = RecordingNotificationState()

  private fun initializeNotification() {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (state.initialized) {
      return
    }

    createNotificationChannel(context)
    state.receiver = RecordingNotificationReceiver(audioAPIModule.get()!!)
    val filter =
      IntentFilter().apply {
        addAction(RecordingNotificationReceiver.ACTION_PAUSE)
        addAction(RecordingNotificationReceiver.ACTION_RESUME)
        addAction(RecordingNotificationReceiver.ACTION_STOP)
      }
    ContextCompat.registerReceiver(
      context,
      state.receiver,
      filter,
      ContextCompat.RECEIVER_NOT_EXPORTED,
    )
    state.initialized = true
  }

  override fun show(options: ReadableMap?): Notification {
    initializeNotification()
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    parseMapFromRN(options)
    return buildNotification(context)
  }

  /**
   * Rebuilds with an updated paused flag, leaving the sticky RN options untouched.
   * Used by native-initiated pause/resume so the action button flips even when JS
   * is unreachable.
   */
  fun rebuildWithPausedState(paused: Boolean): Notification {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    state.paused = paused
    return buildNotification(context)
  }

  private fun buildNotification(context: ReactApplicationContext): Notification {
    // The notification is rebuilt from scratch on every show() so that every option —
    // including the tap intent — reflects the latest values.
    val builder =
      NotificationCompat
        .Builder(context, channelId)
        .setOngoing(true)
        .setOnlyAlertOnce(true)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setContentTitle(state.title)
        .setContentText(state.contentText)
        .setSmallIcon(
          resolveDrawable(context, state.smallIconResourceName) ?: android.R.drawable.ic_btn_speak_now,
        )

    resolveDrawable(context, state.largeIconResourceName)?.let {
      builder.setLargeIcon(Icon.createWithResource(context, it))
    }
    state.backgroundColor?.let { builder.setColor(it) }

    setupContentIntent(context, builder)
    setupActions(context, builder)
    setupChronometer(builder)

    return builder.build()
  }

  private fun setupContentIntent(
    context: Context,
    builder: NotificationCompat.Builder,
  ) {
    val launchIntent = context.packageManager.getLaunchIntentForPackage(context.packageName) ?: return
    state.deepLinkUri?.let {
      // React Native's Linking only surfaces intent data for ACTION_VIEW — with the
      // launcher's ACTION_MAIN the URI would be silently ignored. The intent stays
      // explicit (component set), so no intent filter is consulted.
      launchIntent.action = Intent.ACTION_VIEW
      launchIntent.removeCategory(Intent.CATEGORY_LAUNCHER)
      launchIntent.data = Uri.parse(it)
    }
    builder.setContentIntent(
      PendingIntent.getActivity(
        context,
        REQUEST_CODE_CONTENT,
        launchIntent,
        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
      ),
    )
  }

  private fun setupActions(
    context: Context,
    builder: NotificationCompat.Builder,
  ) {
    if (state.paused) {
      builder.addAction(
        createAction(
          context,
          RecordingNotificationReceiver.ACTION_RESUME,
          REQUEST_CODE_RESUME,
          state.resumeActionTitle ?: "Resume",
        ),
      )
    } else {
      builder.addAction(
        createAction(
          context,
          RecordingNotificationReceiver.ACTION_PAUSE,
          REQUEST_CODE_PAUSE,
          state.pauseActionTitle ?: "Pause",
        ),
      )
    }

    if (state.showStopAction) {
      builder.addAction(
        createAction(
          context,
          RecordingNotificationReceiver.ACTION_STOP,
          REQUEST_CODE_STOP,
          state.stopActionTitle ?: "Stop",
        ),
      )
    }
  }

  private fun createAction(
    context: Context,
    action: String,
    requestCode: Int,
    title: String,
  ): NotificationCompat.Action {
    val intent = Intent(action).apply { `package` = context.packageName }
    val pendingIntent =
      PendingIntent.getBroadcast(
        context,
        requestCode,
        intent,
        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
      )
    return NotificationCompat.Action(null, title, pendingIntent)
  }

  // The system chronometer always ticks against wall time, so the recording's paused
  // spans are carved out by shifting the base (`startedAtMs`) forward on each resume.
  private fun setupChronometer(builder: NotificationCompat.Builder) {
    val now = System.currentTimeMillis()

    if (state.usesChronometer && !state.paused) {
      if (state.startedAtMs == null) {
        state.startedAtMs = now
      }
      state.pausedAtMs?.let { pausedAt ->
        state.startedAtMs = state.startedAtMs!! + (now - pausedAt)
        state.pausedAtMs = null
      }
      builder
        .setWhen(state.startedAtMs!!)
        .setShowWhen(true)
        .setUsesChronometer(true)
    } else {
      if (state.usesChronometer && state.paused && state.pausedAtMs == null) {
        state.pausedAtMs = now
      }
      builder
        .setUsesChronometer(false)
        .setShowWhen(false)
    }
  }

  private fun resolveDrawable(
    context: Context,
    resourceName: String?,
  ): Int? {
    if (resourceName == null) {
      return null
    }
    val resourceId = context.resources.getIdentifier(resourceName, "drawable", context.packageName)
    return if (resourceId != 0) resourceId else null
  }

  private fun parseMapFromRN(options: ReadableMap?) {
    state.title = options.stringOr("title", state.title ?: "Recording Audio")
    state.contentText = options.stringOr("contentText", state.contentText ?: "Audio recording is in progress/paused")
    state.smallIconResourceName = options.stringOr("smallIconResourceName", state.smallIconResourceName)
    state.largeIconResourceName = options.stringOr("largeIconResourceName", state.largeIconResourceName)
    state.backgroundColor = options.intOr("color", state.backgroundColor)
    state.showStopAction = options.boolOr("showStopAction", state.showStopAction)
    state.pauseActionTitle = options.stringOr("pauseActionTitle", state.pauseActionTitle)
    state.resumeActionTitle = options.stringOr("resumeActionTitle", state.resumeActionTitle)
    state.stopActionTitle = options.stringOr("stopActionTitle", state.stopActionTitle)
    state.deepLinkUri = options.stringOr("deepLinkUri", state.deepLinkUri)
    state.usesChronometer = options.boolOr("usesChronometer", state.usesChronometer)
    // Deliberately not sticky — see the [RecordingNotificationState] KDoc.
    state.paused = options.boolOr("paused", false)
  }

  private fun ReadableMap?.stringOr(
    key: String,
    fallback: String?,
  ): String? = if (this?.hasKey(key) == true) getString(key) else fallback

  private fun ReadableMap?.boolOr(
    key: String,
    fallback: Boolean,
  ): Boolean = if (this?.hasKey(key) == true) getBoolean(key) else fallback

  private fun ReadableMap?.intOr(
    key: String,
    fallback: Int?,
  ): Int? = if (this?.hasKey(key) == true) getInt(key) else fallback

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

  override fun hide() {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")
    if (state.receiver != null) {
      context.unregisterReceiver(state.receiver)
      state.receiver = null
    }
    val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    notificationManager.cancel(notificationId)
    state.initialized = false
    state.startedAtMs = null
    state.pausedAtMs = null
  }

  override fun getNotificationId(): Int = notificationId

  override fun getChannelId(): String = channelId
}

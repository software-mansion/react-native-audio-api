package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.drawable.BitmapDrawable
import android.os.Build
import android.provider.ContactsContract
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.util.Log
import android.view.KeyEvent
import androidx.core.app.NotificationCompat
import androidx.media.app.NotificationCompat.MediaStyle
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.bridge.ReadableType
import com.swmansion.audioapi.AudioAPIModule
import java.io.IOException
import java.lang.ref.WeakReference
import java.net.URL

/**
 * RecordingNotification
 *
 * This notification:
 * - Shows recording status and metadata
 * - Supports recording controls (start, stop)
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
    const val MEDIA_BUTTON = "recording_notification_media_button"
    const val PACKAGE_NAME = "com.swmansion.audioapi.recording"
  }

  private var mediaSession: MediaSessionCompat? = null
  private var notificationBuilder: NotificationCompat.Builder? = null
  private var playbackStateBuilder: PlaybackStateCompat.Builder = PlaybackStateCompat.Builder()

  private var enabledControls: Long = 0
  private var isRecording: Boolean = false

  // Metadata
  private var title: String? = null
  private var artwork: Bitmap? = null

  // Actions
  private var startAction: NotificationCompat.Action? = null
  private var stopAction: NotificationCompat.Action? = null

  private var artworkThread: Thread? = null

  override fun init(params: ReadableMap?): Notification {
    val context = reactContext.get() ?: throw IllegalStateException("React context is null")

    // Create notification channel first
    createNotificationChannel()

    // Create MediaSession
    mediaSession = MediaSessionCompat(context, "RecordingNotification")
    mediaSession?.isActive = true

    // Set up media session callbacks
    mediaSession?.setCallback(
      object : MediaSessionCompat.Callback() {
        override fun onCustomAction(action: String, extras: android.os.Bundle?) {
          Log.d(TAG, "MediaSession: onCustomAction ($action)")
          when (action) {
            "START_RECORDING" -> {
              audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("recordingNotificationStart", mapOf())
            }
            "STOP_RECORDING" -> {
              audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("recordingNotificationStop", mapOf())
            }
          }
        }
      },
    )

    // Create notification builder
    notificationBuilder =
      NotificationCompat
        .Builder(context, channelId)
        .setSmallIcon(android.R.drawable.ic_btn_speak_now)
        .setPriority(NotificationCompat.PRIORITY_HIGH)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true) // Make it persistent (can't swipe away)

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

    // Enable default controls
    enableControl("start", true)
    enableControl("stop", true)

    updateMediaStyle()
    updatePlaybackState()

    // Apply initial params if provided
    if (params != null) {
      update(params)
    }

    return buildNotification()
  }

  override fun reset() {
    // Interrupt artwork loading if in progress
    artworkThread?.interrupt()
    artworkThread = null

    // Reset metadata
    title = null
    artwork = null
    isRecording = false

    // Reset media session
    val emptyMetadata = MediaMetadataCompat.Builder().build()
    mediaSession?.setMetadata(emptyMetadata)

    mediaSession?.isActive = false
    mediaSession?.release()
    mediaSession = null
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
      if (control != null) {
        enableControl(control, enabled)
      }
      return buildNotification()
    }

    // Update metadata
    if (options.hasKey("title")) {
      title = options.getString("title")
    }

    // Update recording state
    if (options.hasKey("state")) {
      when (options.getString("state")) {
        "recording" -> {
          isRecording = true
        }
        "stopped" -> {
          isRecording = false
        }
      }
    }

    // Build MediaMetadata
    val metadataBuilder =
      MediaMetadataCompat
        .Builder()
        .putString(MediaMetadataCompat.METADATA_KEY_TITLE, title)

    // Update notification builder
    notificationBuilder?.setContentTitle(title)
    val statusText = if (isRecording) "Recording..." else "Ready to record"
    notificationBuilder?.setContentText(statusText)

    // Handle artwork
    if (options.hasKey("artwork")) {
      artworkThread?.interrupt()

      val artworkUrl: String?
      val isLocal: Boolean

      if (options.getType("artwork") == ReadableType.Map) {
        artworkUrl = options.getMap("artwork")?.getString("uri")
        isLocal = true
      } else {
        artworkUrl = options.getString("artwork")
        isLocal = false
      }

      if (artworkUrl != null) {
        artworkThread =
          Thread {
            try {
              val bitmap = loadArtwork(artworkUrl, isLocal)
              if (bitmap != null) {
                artwork = bitmap
                metadataBuilder.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, bitmap)
                notificationBuilder?.setLargeIcon(bitmap)
              }
              artworkThread = null
            } catch (e: Exception) {
              Log.e(TAG, "Error loading artwork: ${e.message}", e)
            }
          }
        artworkThread?.start()
      }
    }

    mediaSession?.setMetadata(metadataBuilder.build())
    mediaSession?.isActive = true

    // Update ongoing state - only persistent when recording
    notificationBuilder?.setOngoing(isRecording)

    // Update media style to reflect current state
    updatePlaybackState()
    updateMediaStyle()

    return buildNotification()
  }

  private fun buildNotification(): Notification =
    notificationBuilder?.build()
      ?: throw IllegalStateException("Notification not initialized. Call init() first.")

  private fun updatePlaybackState() {
    // Set playback state with custom actions to preserve custom icons
    val state = if (isRecording) PlaybackStateCompat.STATE_PLAYING else PlaybackStateCompat.STATE_STOPPED

    // Clear previous state and rebuild
    playbackStateBuilder = PlaybackStateCompat.Builder()
    playbackStateBuilder.setState(state, 0, 1.0f)

    // Add only the appropriate custom action based on current state
    if (!isRecording && (enabledControls and PlaybackStateCompat.ACTION_PLAY) != 0L) {
      // Show START button when not recording
      val startAction = PlaybackStateCompat.CustomAction.Builder(
        "START_RECORDING",
        "Start Recording",
        android.R.drawable.ic_btn_speak_now
      ).build()
      playbackStateBuilder.addCustomAction(startAction)
    } else if (isRecording && (enabledControls and PlaybackStateCompat.ACTION_PAUSE) != 0L) {
      // Show STOP button when recording
      val stopAction = PlaybackStateCompat.CustomAction.Builder(
        "STOP_RECORDING",
        "Stop Recording",
        android.R.drawable.ic_media_pause
      ).build()
      playbackStateBuilder.addCustomAction(stopAction)
    }

    mediaSession?.setPlaybackState(playbackStateBuilder.build())
  }

  /**
   * Enable or disable a specific control action.
   */
  private fun enableControl(
    name: String,
    enabled: Boolean,
  ) {
    val controlValue =
      when (name) {
        "start" -> PlaybackStateCompat.ACTION_PLAY
        // Use PAUSE action so the system shows a pause button (consistent with PlaybackNotification)
        "stop" -> PlaybackStateCompat.ACTION_PAUSE
        else -> 0L
      }

    if (controlValue == 0L) return

    enabledControls =
      if (enabled) {
        enabledControls or controlValue
      } else {
        enabledControls and controlValue.inv()
      }

    // Update actions
    updateActions()
    updateMediaStyle()
    updatePlaybackState()
  }

  private fun updateActions() {
    val context = reactContext.get() ?: return

    startAction =
      createAction(
        "start",
        "Start Recording",
        android.R.drawable.ic_btn_speak_now, // Microphone icon
        PlaybackStateCompat.ACTION_PLAY,
      )

    stopAction =
      createAction(
        "stop",
        "Stop Recording",
        android.R.drawable.ic_media_pause,
        PlaybackStateCompat.ACTION_PAUSE,
      )
  }

  private fun createAction(
    name: String,
    title: String,
    icon: Int,
    action: Long,
  ): NotificationCompat.Action? {
    val context = reactContext.get() ?: return null

    if ((enabledControls and action) == 0L) {
      return null
    }

    val keyCode = PlaybackStateCompat.toKeyCode(action)
    val intent = Intent(MEDIA_BUTTON)
    intent.putExtra(Intent.EXTRA_KEY_EVENT, KeyEvent(KeyEvent.ACTION_DOWN, keyCode))
    intent.putExtra(ContactsContract.Directory.PACKAGE_NAME, context.packageName)

    val pendingIntent =
      PendingIntent.getBroadcast(
        context,
        keyCode,
        intent,
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
      )

    return NotificationCompat.Action(icon, title, pendingIntent)
  }

  private fun updateMediaStyle() {
    val style = MediaStyle()
    style.setMediaSession(mediaSession?.sessionToken)
    notificationBuilder?.clearActions()
    style.setShowActionsInCompactView(0, 1, 2)
    notificationBuilder?.setStyle(style)
  }

  private fun loadArtwork(
    url: String,
    isLocal: Boolean,
  ): Bitmap? {
    val context = reactContext.get() ?: return null

    return try {
      if (isLocal && !url.startsWith("http")) {
        // Load local resource
        val helper =
          com.facebook.react.views.imagehelper.ResourceDrawableIdHelper
            .getInstance()
        val drawable = helper.getResourceDrawable(context, url)

        if (drawable is BitmapDrawable) {
          drawable.bitmap
        } else {
          BitmapFactory.decodeFile(url)
        }
      } else {
        // Load from URL
        val connection = URL(url).openConnection()
        connection.connect()
        val inputStream = connection.getInputStream()
        val bitmap = BitmapFactory.decodeStream(inputStream)
        inputStream.close()
        bitmap
      }
    } catch (e: IOException) {
      Log.e(TAG, "Failed to load artwork: ${e.message}", e)
      null
    } catch (e: Exception) {
      Log.e(TAG, "Error loading artwork: ${e.message}", e)
      null
    }
  }

  private fun createNotificationChannel() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val context = reactContext.get() ?: return

      val channel =
        android.app
          .NotificationChannel(
            channelId,
            "Audio Recording",
            android.app.NotificationManager.IMPORTANCE_LOW,
          ).apply {
            description = "Recording controls and status"
            setShowBadge(false)
            lockscreenVisibility = Notification.VISIBILITY_PUBLIC
          }

      val notificationManager =
        context.getSystemService(Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
      notificationManager.createNotificationChannel(channel)

      Log.d(TAG, "Notification channel created: $channelId")
    }
  }
}

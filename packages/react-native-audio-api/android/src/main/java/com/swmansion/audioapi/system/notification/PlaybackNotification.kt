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
import androidx.core.graphics.drawable.IconCompat
import androidx.media.app.NotificationCompat.MediaStyle
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.bridge.ReadableType
import com.swmansion.audioapi.AudioAPIModule
import java.io.IOException
import java.lang.ref.WeakReference
import java.net.URL

/**
 * PlaybackNotification
 *
 * This notification:
 * - Shows media metadata (title, artist, album, artwork)
 * - Supports playback controls (play, pause, next, previous, skip)
 * - Integrates with Android MediaSession for lock screen controls
 * - Is persistent and cannot be swiped away when playing
 * - Notifies its dismissal via PlaybackNotificationReceiver
 */
class PlaybackNotification(
  private val reactContext: WeakReference<ReactApplicationContext>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
  private val notificationId: Int,
  private val channelId: String,
) : BaseNotification {
  companion object {
    private const val TAG = "PlaybackNotification"
    const val MEDIA_BUTTON = "playback_notification_media_button"
    const val PACKAGE_NAME = "com.swmansion.audioapi.playback"
  }

  private var mediaSession: MediaSessionCompat? = null
  private var notificationBuilder: NotificationCompat.Builder? = null
  private var playbackStateBuilder: PlaybackStateCompat.Builder = PlaybackStateCompat.Builder()
  private var playbackState: PlaybackStateCompat = playbackStateBuilder.build()
  private var playbackPlayingState: Int = PlaybackStateCompat.STATE_PAUSED

  private var enabledControls: Long = 0
  private var isPlaying: Boolean = false

  // Metadata
  private var title: String? = null
  private var artist: String? = null
  private var album: String? = null
  private var artwork: Bitmap? = null
  private var smallIcon: IconCompat? = null
  private var duration: Long = 0L
  private var elapsedTime: Long = 0L
  private var speed: Float = 1.0F

  // Actions
  private var playAction: NotificationCompat.Action? = null
  private var pauseAction: NotificationCompat.Action? = null
  private var nextAction: NotificationCompat.Action? = null
  private var previousAction: NotificationCompat.Action? = null
  private var skipForwardAction: NotificationCompat.Action? = null
  private var skipBackwardAction: NotificationCompat.Action? = null

  private var artworkThread: Thread? = null
  private var smallIconThread: Thread? = null

  private var isInitialized = false

  private fun initializeIfNeeded() {
    if (isInitialized) return

    val context = reactContext.get() ?: throw IllegalStateException("React context is null")

    // Create notification channel first
    createNotificationChannel()

    // Create MediaSession
    mediaSession = MediaSessionCompat(context, "PlaybackNotification")
    mediaSession?.isActive = true

    // Set up media session callbacks
    mediaSession?.setCallback(
      object : MediaSessionCompat.Callback() {
        override fun onPlay() {
          Log.d(TAG, "MediaSession: onPlay")
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationPlay", mapOf())
        }

        override fun onPause() {
          Log.d(TAG, "MediaSession: onPause")
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationPause", mapOf())
        }

        override fun onSkipToNext() {
          Log.d(TAG, "MediaSession: onSkipToNext")
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationNext", mapOf())
        }

        override fun onSkipToPrevious() {
          Log.d(TAG, "MediaSession: onSkipToPrevious")
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationPrevious", mapOf())
        }

        override fun onFastForward() {
          Log.d(TAG, "MediaSession: onFastForward")
          val body = HashMap<String, Any>().apply { put("value", 15) }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationSkipForward", body)
        }

        override fun onRewind() {
          Log.d(TAG, "MediaSession: onRewind")
          val body = HashMap<String, Any>().apply { put("value", 15) }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationSkipBackward", body)
        }

        override fun onSeekTo(pos: Long) {
          Log.d(TAG, "MediaSession: onSeekTo - position: $pos")
          val body = HashMap<String, Any>().apply { put("value", pos / 1000.0) } // Convert to seconds
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationSeekTo", body)
        }
      },
    )

    // Create notification builder
    notificationBuilder =
      NotificationCompat
        .Builder(context, channelId)
        .setSmallIcon(android.R.drawable.ic_media_play)
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
    val deleteIntent = Intent(PlaybackNotificationReceiver.ACTION_NOTIFICATION_DISMISSED)
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
    enableControl("play", true)
    enableControl("pause", true)
    enableControl("next", true)
    enableControl("previous", true)
    enableControl("seekTo", true)

    updateMediaStyle()
    updatePlaybackState(PlaybackStateCompat.STATE_PAUSED)

    isInitialized = true
  }

  override fun show(params: ReadableMap?): Notification {
    initializeIfNeeded()

    // Apply params if provided (this updates the notification)
    if (params != null) {
      updateInternal(params)
    }

    return buildNotification()
  }

  override fun hide() {
    // Interrupt artwork loading if in progress
    artworkThread?.interrupt()
    artworkThread = null
    smallIconThread?.interrupt()
    smallIconThread = null

    // Reset metadata
    title = null
    artist = null
    album = null
    artwork = null
    smallIcon = null
    duration = 0L
    elapsedTime = 0L
    speed = 1.0F
    isPlaying = false

    // Reset media session
    val emptyMetadata = MediaMetadataCompat.Builder().build()
    mediaSession?.setMetadata(emptyMetadata)

    playbackState =
      playbackStateBuilder
        .setState(PlaybackStateCompat.STATE_NONE, 0, 0f)
        .setActions(enabledControls)
        .build()
    mediaSession?.setPlaybackState(playbackState)
    mediaSession?.isActive = false
    mediaSession?.release()
    mediaSession = null

    notificationBuilder = null
    isInitialized = false
  }

  override fun getNotificationId(): Int = notificationId

  override fun getChannelId(): String = channelId

  private fun updateInternal(options: ReadableMap?) {
    if (options == null) {
      return
    }

    // Handle control enable/disable
    if (options.hasKey("control") && options.hasKey("enabled")) {
      val control = options.getString("control")
      val enabled = options.getBoolean("enabled")
      if (control != null) {
        enableControl(control, enabled)
      }
      return
    }

    // Update metadata
    if (options.hasKey("title")) {
      title = options.getString("title")
    }

    if (options.hasKey("artist")) {
      artist = options.getString("artist")
    }

    if (options.hasKey("album")) {
      album = options.getString("album")
    }

    if (options.hasKey("duration")) {
      duration = (options.getDouble("duration") * 1000).toLong()
    }

    if (options.hasKey("elapsedTime")) {
      elapsedTime = (options.getDouble("elapsedTime") * 1000).toLong()
    } else {
      // Use the current position from the media session controller (live calculated position)
      val controllerPosition = mediaSession?.controller?.playbackState?.position
      if (controllerPosition != null && controllerPosition > 0) {
        elapsedTime = controllerPosition
      }
    }

    if (options.hasKey("speed")) {
      speed = options.getDouble("speed").toFloat()
    } else {
      // Use the current speed from the media session controller
      val controllerSpeed = mediaSession?.controller?.playbackState?.playbackSpeed
      if (controllerSpeed != null && controllerSpeed > 0) {
        speed = controllerSpeed
      }
    }

    // Ensure speed is at least 1.0 when playing
    if (isPlaying && speed == 0f) {
      speed = 1.0f
    }

    // Update playback state
    if (options.hasKey("state")) {
      when (options.getString("state")) {
        "playing" -> {
          playbackPlayingState = PlaybackStateCompat.STATE_PLAYING
        }

        "paused" -> {
          playbackPlayingState = PlaybackStateCompat.STATE_PAUSED
        }
      }
    }

    // Build MediaMetadata
    val metadataBuilder =
      MediaMetadataCompat
        .Builder()
        .putString(MediaMetadataCompat.METADATA_KEY_TITLE, title)
        .putString(MediaMetadataCompat.METADATA_KEY_ARTIST, artist)
        .putString(MediaMetadataCompat.METADATA_KEY_ALBUM, album)
        .putLong(MediaMetadataCompat.METADATA_KEY_DURATION, duration)

    // Update notification builder
    notificationBuilder
      ?.setContentTitle(title)
      ?.setContentText(artist)

    // Handle artwork (large icon)
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
                // Post UI updates to main thread for thread safety
                val context = reactContext.get()
                context?.runOnUiQueueThread {
                  try {
                    artwork = bitmap
                    notificationBuilder?.setLargeIcon(bitmap)

                    // Add artwork to current metadata without touching other fields
                    val currentMetadata = mediaSession?.controller?.metadata
                    if (currentMetadata != null) {
                      val updatedBuilder = MediaMetadataCompat.Builder(currentMetadata)
                      updatedBuilder.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, bitmap)
                      mediaSession?.setMetadata(updatedBuilder.build())
                    }

                    // Refresh the notification on main thread
                    val notificationManager =
                      context.getSystemService(Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
                    notificationManager.notify(notificationId, buildNotification())
                  } catch (e: Exception) {
                    Log.e(TAG, "Error updating notification with artwork: ${e.message}", e)
                  }
                }
              }
              artworkThread = null
            } catch (e: Exception) {
              Log.e(TAG, "Error loading artwork: ${e.message}", e)
              artworkThread = null
            }
          }
        artworkThread?.start()
      }
    }

    // Handle androidSmallIcon (small icon)
    if (options.hasKey("androidSmallIcon")) {
      smallIconThread?.interrupt()

      val smallIconUrl: String?
      val isLocal: Boolean

      if (options.getType("androidSmallIcon") == ReadableType.Map) {
        smallIconUrl = options.getMap("androidSmallIcon")?.getString("uri")
        isLocal = true
      } else {
        smallIconUrl = options.getString("androidSmallIcon")
        isLocal = false
      }

      if (smallIconUrl != null) {
        smallIconThread =
          Thread {
            try {
              val bitmap = loadArtwork(smallIconUrl, isLocal)
              if (bitmap != null) {
                // Post UI updates to main thread for thread safety
                val context = reactContext.get()
                context?.runOnUiQueueThread {
                  try {
                    val icon = IconCompat.createWithBitmap(bitmap)
                    smallIcon = icon
                    notificationBuilder?.setSmallIcon(icon)

                    // Refresh the notification on main thread
                    val notificationManager =
                      context.getSystemService(Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
                    notificationManager.notify(notificationId, buildNotification())
                  } catch (e: Exception) {
                    Log.e(TAG, "Error updating notification with small icon: ${e.message}", e)
                  }
                }
              }
              smallIconThread = null
            } catch (e: Exception) {
              Log.e(TAG, "Error loading small icon: ${e.message}", e)
              smallIconThread = null
            }
          }
        smallIconThread?.start()
      }
    }

    updatePlaybackState(playbackPlayingState)
    mediaSession?.setMetadata(metadataBuilder.build())
    mediaSession?.isActive = true
  }

  private fun buildNotification(): Notification =
    notificationBuilder?.build()
      ?: throw IllegalStateException("Notification not initialized")

  /**
   * Enable or disable a specific control action.
   */
  private fun enableControl(
    name: String,
    enabled: Boolean,
  ) {
    val controlValue =
      when (name) {
        "play" -> PlaybackStateCompat.ACTION_PLAY
        "pause" -> PlaybackStateCompat.ACTION_PAUSE
        "next" -> PlaybackStateCompat.ACTION_SKIP_TO_NEXT
        "previous" -> PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS
        "skipForward" -> PlaybackStateCompat.ACTION_FAST_FORWARD
        "skipBackward" -> PlaybackStateCompat.ACTION_REWIND
        "seekTo" -> PlaybackStateCompat.ACTION_SEEK_TO
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

    // Update playback state with new controls
    playbackState =
      playbackStateBuilder
        .setActions(enabledControls)
        .build()
    mediaSession?.setPlaybackState(playbackState)
  }

  private fun updateActions() {
    val context = reactContext.get() ?: return
    val packageName = context.packageName

    playAction =
      createAction(
        "play",
        "Play",
        android.R.drawable.ic_media_play,
        PlaybackStateCompat.ACTION_PLAY,
      )

    pauseAction =
      createAction(
        "pause",
        "Pause",
        android.R.drawable.ic_media_pause,
        PlaybackStateCompat.ACTION_PAUSE,
      )

    nextAction =
      createAction(
        "next",
        "Next",
        android.R.drawable.ic_media_next,
        PlaybackStateCompat.ACTION_SKIP_TO_NEXT,
      )

    previousAction =
      createAction(
        "previous",
        "Previous",
        android.R.drawable.ic_media_previous,
        PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS,
      )

    skipForwardAction =
      createAction(
        "skip_forward",
        "Skip Forward",
        android.R.drawable.ic_media_ff,
        PlaybackStateCompat.ACTION_FAST_FORWARD,
      )

    skipBackwardAction =
      createAction(
        "skip_backward",
        "Skip Backward",
        android.R.drawable.ic_media_rew,
        PlaybackStateCompat.ACTION_REWIND,
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

  private fun updatePlaybackState(state: Int) {
    isPlaying = state == PlaybackStateCompat.STATE_PLAYING

    playbackState =
      playbackStateBuilder
        .setState(state, elapsedTime, speed)
        .setActions(enabledControls)
        .build()
    if (mediaSession != null) {
      Log.d(TAG, "mediaSession is not null")
    } else {
      Log.d(TAG, "mediaSession is null")
    }
    mediaSession?.setPlaybackState(playbackState)

    // Update ongoing state - only persistent when playing
    notificationBuilder?.setOngoing(isPlaying)
  }

  private fun updateMediaStyle() {
    val style = MediaStyle()
    style.setMediaSession(mediaSession?.sessionToken)

    // Clear existing actions
    notificationBuilder?.clearActions()

    // Add actions in order based on enabled controls
    val compactActions = mutableListOf<Int>()
    var actionIndex = 0

    if (previousAction != null) {
      notificationBuilder?.addAction(previousAction)
      actionIndex++
    }

    if (skipBackwardAction != null) {
      notificationBuilder?.addAction(skipBackwardAction)
      actionIndex++
    }

    if (playAction != null && !isPlaying) {
      notificationBuilder?.addAction(playAction)
      compactActions.add(actionIndex)
      actionIndex++
    }

    if (pauseAction != null && isPlaying) {
      notificationBuilder?.addAction(pauseAction)
      compactActions.add(actionIndex)
      actionIndex++
    }

    if (skipForwardAction != null) {
      notificationBuilder?.addAction(skipForwardAction)
      actionIndex++
    }

    if (nextAction != null) {
      notificationBuilder?.addAction(nextAction)
      actionIndex++
    }

    // Show up to 3 actions in compact view
    style.setShowActionsInCompactView(*compactActions.take(3).toIntArray())
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
            "Audio Playback",
            android.app.NotificationManager.IMPORTANCE_LOW,
          ).apply {
            description = "Media playback controls and information"
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

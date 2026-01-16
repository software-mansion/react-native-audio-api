package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.drawable.BitmapDrawable
import android.os.Build
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.KeyEvent
import androidx.core.app.NotificationCompat
import androidx.media.app.NotificationCompat.MediaStyle
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.bridge.ReadableType
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.R
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
    const val MEDIA_BUTTON = "playback_notification_media_button"
    const val ACTION_SKIP_FORWARD = "com.swmansion.audioapi.ACTION_SKIP_FORWARD"
    const val ACTION_SKIP_BACKWARD = "com.swmansion.audioapi.ACTION_SKIP_BACKWARD"
    const val ID = 100
  }

  private var mediaSession: MediaSessionCompat? = null
  private var notificationBuilder: NotificationCompat.Builder? = null
  private var pb: PlaybackStateCompat.Builder = PlaybackStateCompat.Builder()
  private var state: PlaybackStateCompat = pb.build()
  private var controls: Long = 0

  private var isPlaying: Boolean = false
  private var isInitialized = false

  // Metadata
  private var title: String? = null
  private var artist: String? = null
  private var album: String? = null
  private var artwork: Bitmap? = null
  private var duration: Long = 0L
  private var elapsedTime: Long = 0L
  private var speed: Float = 1.0F
  private var playbackStateVal: Int = PlaybackStateCompat.STATE_PAUSED

  private var artworkThread: Thread? = null

  private fun initializeIfNeeded() {
    if (isInitialized) return
    val context = reactContext.get() ?: return

    createNotificationChannel()

    mediaSession = MediaSessionCompat(context, "PlaybackNotification")

    mediaSession?.setCallback(
      object : MediaSessionCompat.Callback() {
        override fun onPlay() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationPlay", mapOf())
        }

        override fun onPause() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationPause", mapOf())
        }

        override fun onSkipToNext() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationNext", mapOf())
        }

        override fun onSkipToPrevious() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationPrevious", mapOf())
        }

        override fun onFastForward() {
          val body = HashMap<String, Any>().apply { put("value", 15) }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationSkipForward", body)
        }

        override fun onRewind() {
          val body = HashMap<String, Any>().apply { put("value", 15) }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationSkipBackward", body)
        }

        override fun onSeekTo(pos: Long) {
          val body = HashMap<String, Any>().apply { put("value", pos / 1000.0) }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("playbackNotificationSeekTo", body)
        }

        override fun onCustomAction(
          action: String?,
          extras: android.os.Bundle?,
        ) {
          if (action == "SkipForward") {
            onFastForward()
          } else if (action == "SkipBackward") {
            onRewind()
          }
        }
      },
    )

    notificationBuilder =
      NotificationCompat
        .Builder(context, channelId)
        .setSmallIcon(android.R.drawable.ic_media_play)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setPriority(NotificationCompat.PRIORITY_HIGH)

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

    pb.setActions(controls)
    mediaSession?.isActive = true

    isInitialized = true
  }

  override fun show(options: ReadableMap?): Notification {
    initializeIfNeeded()
    if (options != null) {
      updateInternal(options)
    }
    return buildNotification()
  }

  override fun hide() {
    if (!isInitialized) return

    if (artworkThread != null && artworkThread!!.isAlive) {
      artworkThread!!.interrupt()
    }
    artworkThread = null

    mediaSession?.isActive = false
    mediaSession?.release()
    mediaSession = null
    notificationBuilder = null
    isInitialized = false

    controls = 0
    isPlaying = false
    artwork = null
  }

  override fun getNotificationId(): Int = notificationId

  override fun getChannelId(): String = channelId

  private fun updateInternal(info: ReadableMap) {
    if (info.hasKey("control") && info.hasKey("enabled")) {
      enableControl(info.getString("control"), info.getBoolean("enabled"))
    }

    val md = MediaMetadataCompat.Builder()

    if (info.hasKey("title")) title = info.getString("title")
    if (info.hasKey("artist")) artist = info.getString("artist")
    if (info.hasKey("album")) album = info.getString("album")
    if (info.hasKey("duration")) duration = (info.getDouble("duration") * 1000).toLong()

    md.putString(MediaMetadataCompat.METADATA_KEY_TITLE, title)
    md.putString(MediaMetadataCompat.METADATA_KEY_ARTIST, artist)
    md.putString(MediaMetadataCompat.METADATA_KEY_ALBUM, album)
    md.putLong(MediaMetadataCompat.METADATA_KEY_DURATION, duration)

    notificationBuilder?.setContentTitle(title)
    notificationBuilder?.setContentText(artist)
    notificationBuilder?.setContentInfo(album)

    if (info.hasKey("artwork")) {
      if (artworkThread != null && artworkThread!!.isAlive) {
        artworkThread!!.interrupt()
      }

      var localArtwork = false
      val artworkUri =
        if (info.getType("artwork") == ReadableType.Map) {
          localArtwork = true
          info.getMap("artwork")?.getString("uri")
        } else {
          info.getString("artwork")
        }

      if (artworkUri != null) {
        artworkThread =
          Thread {
            try {
              val bitmap = loadArtwork(artworkUri, localArtwork)
              if (bitmap != null) {
                artwork = bitmap
                val context = reactContext.get()
                context?.runOnUiQueueThread {
                  notificationBuilder?.setLargeIcon(bitmap)

                  val currentMetadata = mediaSession?.controller?.metadata
                  val newBuilder = MediaMetadataCompat.Builder(currentMetadata ?: MediaMetadataCompat.Builder().build())
                  mediaSession?.setMetadata(newBuilder.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, bitmap).build())

                  // Trigger update
                  val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
                  notificationManager.notify(notificationId, buildNotification())
                }
              }
            } catch (ex: Exception) {
              ex.printStackTrace()
            }
          }
        artworkThread!!.start()
      }
    }

    if (info.hasKey("speed")) {
      speed = info.getDouble("speed").toFloat()
    }

    if (isPlaying && speed == 0F) {
      speed = 1F
    }

    if (info.hasKey("elapsedTime")) {
      elapsedTime = (info.getDouble("elapsedTime") * 1000).toLong()
    } else {
      if (state.position != PlaybackStateCompat.PLAYBACK_POSITION_UNKNOWN) {
        elapsedTime = state.position
      }
    }

    if (info.hasKey("state")) {
      when (info.getString("state")) {
        "playing", "state_playing" -> playbackStateVal = PlaybackStateCompat.STATE_PLAYING
        "paused", "state_paused" -> playbackStateVal = PlaybackStateCompat.STATE_PAUSED
      }
    }

    updatePlaybackState(playbackStateVal)

    if (artwork != null) {
      md.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, artwork)
    }
    mediaSession?.setMetadata(md.build())

    updateNotificationsActions()
  }

  private fun enableControl(
    name: String?,
    enabled: Boolean,
  ) {
    if (name == null) return
    var controlValue = 0L
    when (name) {
      "play", "remotePlay" -> controlValue = PlaybackStateCompat.ACTION_PLAY
      "pause", "remotePause" -> controlValue = PlaybackStateCompat.ACTION_PAUSE
      "stop", "remoteStop" -> controlValue = PlaybackStateCompat.ACTION_STOP
      "togglePlayPause", "remoteTogglePlayPause" -> controlValue = PlaybackStateCompat.ACTION_PLAY_PAUSE
      "next", "remoteNextTrack" -> controlValue = PlaybackStateCompat.ACTION_SKIP_TO_NEXT
      "previous", "remotePreviousTrack" -> controlValue = PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS
      "skipForward", "remoteSkipForward" -> controlValue = PlaybackStateCompat.ACTION_FAST_FORWARD
      "skipBackward", "remoteSkipBackward" -> controlValue = PlaybackStateCompat.ACTION_REWIND
      "seekTo", "remoteChangePlaybackPosition" -> controlValue = PlaybackStateCompat.ACTION_SEEK_TO
    }

    controls =
      if (enabled) {
        controls or controlValue
      } else {
        controls and controlValue.inv()
      }

    updatePlaybackActionState()
    updateNotificationsActions()
  }

  private fun updatePlaybackActionState() {
    val builder = PlaybackStateCompat.Builder()
    builder.setActions(controls)

    if (hasControl(PlaybackStateCompat.ACTION_REWIND)) {
      builder.addCustomAction(
        PlaybackStateCompat.CustomAction
          .Builder(
            "SkipBackward",
            "Skip Backward",
            R.drawable.skip_backward_15,
          ).build(),
      )
    }

    if (hasControl(PlaybackStateCompat.ACTION_FAST_FORWARD)) {
      builder.addCustomAction(
        PlaybackStateCompat.CustomAction
          .Builder(
            "SkipForward",
            "Skip Forward",
            R.drawable.skip_forward_15,
          ).build(),
      )
    }

    pb = builder
  }

  private fun updatePlaybackState(playbackStateCode: Int) {
    isPlaying = playbackStateCode == PlaybackStateCompat.STATE_PLAYING

    pb.setState(playbackStateCode, elapsedTime, speed)
    state = pb.build()
    mediaSession?.setPlaybackState(state)

    notificationBuilder?.setOngoing(isPlaying)
  }

  private fun updateNotificationsActions() {
    notificationBuilder?.clearActions()

    val style = MediaStyle()
    style.setMediaSession(mediaSession?.sessionToken)

    val context = reactContext.get() ?: return

    var index = 0
    val actionsList = mutableListOf<Int>()

    if (hasControl(PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS)) {
      notificationBuilder?.addAction(
        createAction("previous", "Previous", android.R.drawable.ic_media_previous, PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS),
      )
      actionsList.add(index++)
    }

    if (hasControl(PlaybackStateCompat.ACTION_REWIND)) {
      notificationBuilder?.addAction(
        createAction("skip_backward", "Skip Backward", R.drawable.skip_backward_15, PlaybackStateCompat.ACTION_REWIND),
      )
      actionsList.add(index++)
    }

    if (isPlaying) {
      if (hasControl(PlaybackStateCompat.ACTION_PAUSE)) {
        notificationBuilder?.addAction(
          createAction("pause", "Pause", android.R.drawable.ic_media_pause, PlaybackStateCompat.ACTION_PAUSE),
        )
        actionsList.add(index++)
      }
    } else {
      if (hasControl(PlaybackStateCompat.ACTION_PLAY)) {
        notificationBuilder?.addAction(createAction("play", "Play", android.R.drawable.ic_media_play, PlaybackStateCompat.ACTION_PLAY))
        actionsList.add(index++)
      }
    }

    if (hasControl(PlaybackStateCompat.ACTION_FAST_FORWARD)) {
      notificationBuilder?.addAction(
        createAction("skip_forward", "Skip Forward", R.drawable.skip_forward_15, PlaybackStateCompat.ACTION_FAST_FORWARD),
      )
      actionsList.add(index++)
    }

    if (hasControl(PlaybackStateCompat.ACTION_SKIP_TO_NEXT)) {
      notificationBuilder?.addAction(
        createAction("next", "Next", android.R.drawable.ic_media_next, PlaybackStateCompat.ACTION_SKIP_TO_NEXT),
      )
      actionsList.add(index++)
    }

    if (actionsList.size > 3) {
      style.setShowActionsInCompactView(actionsList[0], actionsList[1], actionsList[2])
    } else {
      style.setShowActionsInCompactView(*actionsList.toIntArray())
    }

    notificationBuilder?.setStyle(style)
  }

  private fun createAction(
    name: String,
    title: String,
    icon: Int,
    mediaAction: Long,
  ): NotificationCompat.Action {
    val context = reactContext.get()!!
    val pendingIntent: PendingIntent

    if (name == "skip_forward" || name == "skip_backward") {
      val customActionName = if (name == "skip_forward") ACTION_SKIP_FORWARD else ACTION_SKIP_BACKWARD
      val intent = Intent(customActionName)
      intent.setPackage(context.packageName)
      pendingIntent =
        PendingIntent.getBroadcast(
          context,
          if (name == "skip_forward") 1001 else 1002,
          intent,
          PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
    } else {
      val keyCode = PlaybackStateCompat.toKeyCode(mediaAction)
      val intent = Intent(MEDIA_BUTTON)
      intent.setPackage(context.packageName)
      intent.putExtra(Intent.EXTRA_KEY_EVENT, KeyEvent(KeyEvent.ACTION_DOWN, keyCode))
      pendingIntent =
        PendingIntent.getBroadcast(
          context,
          keyCode,
          intent,
          PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
    }
    return NotificationCompat.Action(icon, title, pendingIntent)
  }

  private fun hasControl(control: Long): Boolean = (controls and control) == control

  private fun loadArtwork(
    url: String,
    local: Boolean,
  ): Bitmap? {
    val context = reactContext.get() ?: return null

    return try {
      if (local && !url.startsWith("http")) {
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
        val connection = URL(url).openConnection()
        connection.connect()
        val inputStream = connection.getInputStream()
        val bitmap = BitmapFactory.decodeStream(inputStream)
        inputStream.close()
        bitmap
      }
    } catch (e: IOException) {
      null
    } catch (e: Exception) {
      null
    }
  }

  private fun createNotificationChannel() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val context = reactContext.get() ?: return
      val manager = context.getSystemService(Context.NOTIFICATION_SERVICE) as android.app.NotificationManager
      val channel =
        android.app.NotificationChannel(
          channelId,
          "Media Playback",
          android.app.NotificationManager.IMPORTANCE_LOW,
        )
      channel.description = "Media playback controls"
      channel.setShowBadge(false)
      channel.lockscreenVisibility = Notification.VISIBILITY_PUBLIC
      manager.createNotificationChannel(channel)
    }
  }

  private fun buildNotification(): Notification =
    notificationBuilder?.build() ?: throw IllegalStateException("Notification not initialized")
}

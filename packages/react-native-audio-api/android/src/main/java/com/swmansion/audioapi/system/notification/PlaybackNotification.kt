package com.swmansion.audioapi.system.notification

import android.app.Notification
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import android.os.Build
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.KeyEvent
import androidx.core.app.NotificationCompat
import androidx.core.net.toUri
import androidx.media.app.NotificationCompat.MediaStyle
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.bridge.ReadableType
import com.facebook.react.views.imagehelper.ResourceDrawableIdHelper
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.R
import com.swmansion.audioapi.system.AudioEvent
import java.io.File
import java.lang.ref.WeakReference

/**
 * PlaybackNotification
 *
 * This notification:
 * - Shows media metadata (title, artist, album, artwork)
 * - Supports playback controls (play, pause, next, previous, skip)
 * - Integrates with Android MediaSession for lock screen controls
 * - Is persistent and cannot be swiped away when playing
 * - Notifies its dismissal via PlaybackNotificationReceiver
 *
 * Every mutable field below is confined to the React NativeModules queue thread.
 * Artwork arrives asynchronously and is therefore delivered back onto the
 * same queue rather than onto the JS or main thread.
 */
class PlaybackNotification(
  private val reactContext: WeakReference<ReactApplicationContext>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
  private val notificationId: Int,
  private val channelId: String,
  private val artworkLoader: ArtworkLoader,
  private val notificationRedisplay: NotificationRedisplay,
) : BaseNotification {
  companion object {
    const val MEDIA_BUTTON = "playback_notification_media_button"
    const val ACTION_SKIP_FORWARD = "com.swmansion.audioapi.ACTION_SKIP_FORWARD"
    const val ACTION_SKIP_BACKWARD = "com.swmansion.audioapi.ACTION_SKIP_BACKWARD"
    const val ID = 100

    // Must match kDefaultSkipIntervalSeconds on iOS.
    const val DEFAULT_SKIP_INTERVAL_SECONDS = 15
    private const val ARTWORK_TARGET_SIZE_PX = 512
  }

  private var skipIntervalSeconds: Int = DEFAULT_SKIP_INTERVAL_SECONDS

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

  private var artworkRequest: ArtworkLoader.Handle? = null

  /** The artwork currently shown or in flight; the key that de-duplicates repeated updates. */
  private var displayedArtworkUri: Uri? = null

  /**
   * Incremented by every new artwork request and by [hide].
   *
   * A load captures the generation it started with and its result is dropped if that no longer matches.
   */
  private var artworkGeneration: Long = 0

  private fun initializeIfNeeded() {
    if (isInitialized) return
    val context = reactContext.get() ?: return

    createNotificationChannel()

    mediaSession = MediaSessionCompat(context, "PlaybackNotification")

    mediaSession?.setCallback(
      object : MediaSessionCompat.Callback() {
        override fun onPlay() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_PLAY.ordinal, mapOf())
        }

        override fun onPause() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_PAUSE.ordinal, mapOf())
        }

        override fun onStop() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_STOP.ordinal, mapOf())
        }

        override fun onSkipToNext() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_NEXT_TRACK.ordinal, mapOf())
        }

        override fun onSkipToPrevious() {
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_PREVIOUS_TRACK.ordinal, mapOf())
        }

        override fun onFastForward() {
          val body = HashMap<String, Any>().apply { put("value", skipIntervalSeconds) }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_SKIP_FORWARD.ordinal, body)
        }

        override fun onRewind() {
          val body = HashMap<String, Any>().apply { put("value", skipIntervalSeconds) }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_SKIP_BACKWARD.ordinal, body)
        }

        override fun onSeekTo(pos: Long) {
          val body = HashMap<String, Any>().apply { put("value", pos / 1000.0) }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.PLAYBACK_NOTIFICATION_SEEK_TO.ordinal, body)
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
    if (options != null) {
      updateSkipIntervalFromOptions(options)
    }
    initializeIfNeeded()
    if (options != null) {
      updateInternal(options)
    }
    return buildNotification()
  }

  override fun hide() {
    if (!isInitialized) return

    // Invalidates any load already on its way back to this queue; see artworkGeneration.
    artworkGeneration++
    artworkRequest?.cancel()
    artworkRequest = null
    displayedArtworkUri = null

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

  private fun updateSkipIntervalFromOptions(info: ReadableMap) {
    if (info.hasKey("skipInterval")) {
      skipIntervalSeconds = info.getDouble("skipInterval").toInt()
      if (isInitialized) {
        refreshSkipControlIcons()
      }
    }
  }

  private fun skipForwardIcon(): Int =
    if (skipIntervalSeconds == DEFAULT_SKIP_INTERVAL_SECONDS) {
      R.drawable.skip_forward_15
    } else {
      R.drawable.skip_forward
    }

  private fun skipBackwardIcon(): Int =
    if (skipIntervalSeconds == DEFAULT_SKIP_INTERVAL_SECONDS) {
      R.drawable.skip_backward_15
    } else {
      R.drawable.skip_backward
    }

  private fun refreshSkipControlIcons() {
    if (hasControl(PlaybackStateCompat.ACTION_REWIND) ||
      hasControl(PlaybackStateCompat.ACTION_FAST_FORWARD)
    ) {
      updatePlaybackActionState()
      updatePlaybackState(playbackStateVal)
    }
    updateNotificationsActions()
  }

  private fun updateInternal(info: ReadableMap) {
    updateSkipIntervalFromOptions(info)

    if (info.hasKey("control") && info.hasKey("enabled")) {
      enableControl(info.getString("control"), info.getBoolean("enabled"))
      return
    }

    if (info.hasKey("title")) title = info.getString("title")
    if (info.hasKey("artist")) artist = info.getString("artist")
    if (info.hasKey("album")) album = info.getString("album")
    if (info.hasKey("duration")) duration = (info.getDouble("duration") * 1000).toLong()

    if (info.hasKey("androidSmallIcon")) {
      val smallIcon = resolveSmallIconResource(info)
      if (smallIcon != 0) {
        notificationBuilder?.setSmallIcon(smallIcon)
      }
    }

    if (info.hasKey("artwork")) {
      val artworkUri = resolveArtworkUri(info)
      if (artworkUri != null && artworkUri != displayedArtworkUri) {
        artworkRequest?.cancel()
        displayedArtworkUri = artworkUri

        val generation = ++artworkGeneration
        artworkRequest =
          artworkLoader.load(artworkUri, ARTWORK_TARGET_SIZE_PX) { bitmap ->
            onArtworkLoaded(generation, bitmap)
          }
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

    publishMetadata()

    updateNotificationsActions()
  }

  private fun publishMetadata() {
    val session = mediaSession ?: return

    session.setMetadata(
      MediaMetadataCompat
        .Builder()
        .putString(MediaMetadataCompat.METADATA_KEY_TITLE, title)
        .putString(MediaMetadataCompat.METADATA_KEY_ARTIST, artist)
        .putString(MediaMetadataCompat.METADATA_KEY_ALBUM, album)
        .putLong(MediaMetadataCompat.METADATA_KEY_DURATION, duration)
        .apply { artwork?.let { putBitmap(MediaMetadataCompat.METADATA_KEY_ART, it) } }
        .build(),
    )
  }

  private fun onArtworkLoaded(
    generation: Long,
    bitmap: Bitmap?,
  ) {
    if (generation != artworkGeneration) return
    artworkRequest = null

    if (bitmap == null) {
      // Clearing the key lets the same artwork be retried after a transient failure.
      displayedArtworkUri = null
      return
    }

    artwork = bitmap

    val builder = notificationBuilder ?: return
    builder.setLargeIcon(bitmap)
    publishMetadata()
    notificationRedisplay.redisplay(builder.build())
  }

  private fun enableControl(
    name: String?,
    enabled: Boolean,
  ) {
    if (name == null) return
    var controlValue = 0L
    when (name) {
      "play" -> controlValue = PlaybackStateCompat.ACTION_PLAY
      "pause" -> controlValue = PlaybackStateCompat.ACTION_PAUSE
      "stop" -> controlValue = PlaybackStateCompat.ACTION_STOP
      "nextTrack" -> controlValue = PlaybackStateCompat.ACTION_SKIP_TO_NEXT
      "previousTrack" -> controlValue = PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS
      "skipForward" -> controlValue = PlaybackStateCompat.ACTION_FAST_FORWARD
      "skipBackward" -> controlValue = PlaybackStateCompat.ACTION_REWIND
      "seekTo" -> controlValue = PlaybackStateCompat.ACTION_SEEK_TO
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
            skipBackwardIcon(),
          ).build(),
      )
    }

    if (hasControl(PlaybackStateCompat.ACTION_FAST_FORWARD)) {
      builder.addCustomAction(
        PlaybackStateCompat.CustomAction
          .Builder(
            "SkipForward",
            "Skip Forward",
            skipForwardIcon(),
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
        createAction("previousTrack", "Previous track", PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS),
      )
      actionsList.add(index++)
    }

    if (hasControl(PlaybackStateCompat.ACTION_REWIND)) {
      notificationBuilder?.addAction(
        createAction("skip_backward", "Skip Backward", PlaybackStateCompat.ACTION_REWIND),
      )
      actionsList.add(index++)
    }

    if (isPlaying) {
      if (hasControl(PlaybackStateCompat.ACTION_PAUSE)) {
        notificationBuilder?.addAction(
          createAction("pause", "Pause", PlaybackStateCompat.ACTION_PAUSE),
        )
        actionsList.add(index++)
      } else if (hasControl(PlaybackStateCompat.ACTION_STOP)) {
        notificationBuilder?.addAction(
          createAction("stop", "Stop", PlaybackStateCompat.ACTION_STOP),
        )
        actionsList.add(index++)
      }
    } else {
      if (hasControl(PlaybackStateCompat.ACTION_PLAY)) {
        notificationBuilder?.addAction(createAction("play", "Play", PlaybackStateCompat.ACTION_PLAY))
        actionsList.add(index++)
      }
    }

    if (hasControl(PlaybackStateCompat.ACTION_FAST_FORWARD)) {
      notificationBuilder?.addAction(
        createAction("skip_forward", "Skip Forward", PlaybackStateCompat.ACTION_FAST_FORWARD),
      )
      actionsList.add(index++)
    }

    if (hasControl(PlaybackStateCompat.ACTION_SKIP_TO_NEXT)) {
      notificationBuilder?.addAction(
        createAction("nextTrack", "Next track", PlaybackStateCompat.ACTION_SKIP_TO_NEXT),
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
    mediaAction: Long,
  ): NotificationCompat.Action {
    val context = reactContext.get()!!
    val pendingIntent: PendingIntent

    if (name == "skip_forward" || name == "skip_backward") {
      val customActionName = if (name == "skip_forward") ACTION_SKIP_FORWARD else ACTION_SKIP_BACKWARD
      val intent = Intent(customActionName)
      intent.setPackage(context.packageName)
      intent.putExtra(PlaybackNotificationReceiver.EXTRA_SKIP_INTERVAL_SECONDS, skipIntervalSeconds)
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
    return NotificationCompat.Action(actionIcon(name), title, pendingIntent)
  }

  /**
   * Icon drawn on the notification's own action button.
   *
   * Only Android 12 and below show these. From Android 13 the system builds media controls from the
   * PlaybackState and draws its own glyphs for the standard transport actions; the only app-supplied
   * icons that survive there are the ones attached to the custom actions in
   * [updatePlaybackActionState]. This mapping becomes dead once minSdk reaches 33.
   */
  private fun actionIcon(name: String): Int =
    when (name) {
      "play" -> android.R.drawable.ic_media_play
      "pause" -> android.R.drawable.ic_media_pause
      "stop" -> R.drawable.stop
      "nextTrack" -> android.R.drawable.ic_media_next
      "previousTrack" -> android.R.drawable.ic_media_previous
      "skip_forward" -> skipForwardIcon()
      "skip_backward" -> skipBackwardIcon()
      else -> 0
    }

  private fun hasControl(control: Long): Boolean = (controls and control) == control

  private fun resolveSmallIconResource(info: ReadableMap): Int {
    val context = reactContext.get() ?: return 0

    val name =
      if (info.getType("androidSmallIcon") == ReadableType.Map) {
        info.getMap("androidSmallIcon")?.getString("uri")
      } else {
        info.getString("androidSmallIcon")
      }
    if (name.isNullOrEmpty()) return 0

    return ResourceDrawableIdHelper
      .getResourceDrawableId(context, name)
  }

  private fun resolveArtworkUri(info: ReadableMap): Uri? {
    val context = reactContext.get() ?: return null

    val isBundledAsset = info.getType("artwork") == ReadableType.Map
    val source =
      if (isBundledAsset) {
        info.getMap("artwork")?.getString("uri")
      } else {
        info.getString("artwork")
      }
    if (source.isNullOrEmpty()) return null

    if (isBundledAsset && !source.startsWith("http")) {
      // Yields Uri.EMPTY, never null, when the drawable cannot be resolved.
      val drawableUri =
        ResourceDrawableIdHelper
          .getResourceDrawableUri(context, source)
      if (drawableUri != Uri.EMPTY) return drawableUri
    }

    return if (source.startsWith("/")) Uri.fromFile(File(source)) else source.toUri()
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

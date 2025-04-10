package com.swmansion.audioapi.system

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.drawable.BitmapDrawable
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.media.app.NotificationCompat.MediaStyle
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.bridge.ReadableType
import com.facebook.react.views.imagehelper.ResourceDrawableIdHelper.Companion.instance
import java.io.IOException
import java.net.URL

class LockScreenManager(
  private val reactContext: ReactApplicationContext,
  private val mediaSession: MediaSessionCompat,
  private val mediaNotificationManager: MediaNotificationManager,
  val channelId: String,
) {
  private var pb: PlaybackStateCompat.Builder = PlaybackStateCompat.Builder()
  private var state: PlaybackStateCompat = pb.build()
  private var controls: Long = 0
  var isPlaying: Boolean = false

  private var nb: NotificationCompat.Builder = NotificationCompat.Builder(reactContext, channelId)

  private var artworkThread: Thread? = null

  private var title: String? = null
  private var artist: String? = null
  private var album: String? = null
  private var description: String? = null
  private var duration: Long = 0
  private var speed: Float = 1.0F
  private var elapsedTime: Long = 0L
  private var artwork: String? = null
  private var playbackState: Int = PlaybackStateCompat.STATE_PAUSED

  init {
    this.pb.setActions(controls)

    this.nb.setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
    this.nb.setPriority(NotificationCompat.PRIORITY_HIGH)

    updateNotificationMediaStyle()

    mediaNotificationManager.updateActions(controls)
  }

  fun setLockScreenInfo(info: ReadableMap?) {
    if (artworkThread != null && artworkThread!!.isAlive) artworkThread!!.interrupt()
    artworkThread = null

    if (info == null) {
      return
    }

    val md = MediaMetadataCompat.Builder()

    if (info.hasKey("title")) {
      title = info.getString("title")
    }

    if (info.hasKey("artist")) {
      artist = info.getString("artist")
    }

    if (info.hasKey("album")) {
      album = info.getString("album")
    }

    if (info.hasKey("description")) {
      description = info.getString("description")
    }

    if (info.hasKey("duration")) {
      duration = (info.getDouble("duration") * 1000).toLong()
    }

    md.putText(MediaMetadataCompat.METADATA_KEY_TITLE, title)
    md.putText(MediaMetadataCompat.METADATA_KEY_ARTIST, artist)
    md.putText(MediaMetadataCompat.METADATA_KEY_ALBUM, album)
    md.putText(MediaMetadataCompat.METADATA_KEY_DISPLAY_DESCRIPTION, description)
    md.putLong(MediaMetadataCompat.METADATA_KEY_DURATION, duration)

    nb.setContentTitle(title)
    nb.setContentText(artist)
    nb.setContentInfo(album)

    if (info.hasKey("artwork")) {
      var localArtwork = false

      if (info.getType("artwork") == ReadableType.Map) {
        artwork = info.getMap("artwork")?.getString("uri")
        localArtwork = true
      } else {
        artwork = info.getString("artwork")
      }

      val artworkLocal = localArtwork

      artworkThread =
        Thread {
          try {
            val bitmap: Bitmap? = artwork?.let { loadArtwork(it, artworkLocal) }

            val currentMetadata: MediaMetadataCompat = mediaSession.controller.metadata
            val newBuilder =
              MediaMetadataCompat.Builder(
                currentMetadata,
              )
            mediaSession.setMetadata(
              newBuilder.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, bitmap).build(),
            )

            nb.setLargeIcon(bitmap)
            mediaNotificationManager.show(nb, isPlaying)

            artworkThread = null
          } catch (ex: Exception) {
            ex.printStackTrace()
          }
        }
      artworkThread!!.start()
    } else {
      md.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, null)
      nb.setLargeIcon(null as Bitmap?)
    }

    speed =
      if (info.hasKey("speed")) {
        info.getDouble("speed").toFloat()
      } else {
        state.playbackSpeed
      }

    elapsedTime =
      if (info.hasKey("elapsedTime")) {
        info.getDouble("elapsedTime").toLong()
      } else {
        state.position
      }

    if (info.hasKey("state")) {
      val state = info.getString("state")

      when (state) {
        "state_playing" -> {
          this.playbackState = PlaybackStateCompat.STATE_PLAYING
        }
        "state_paused" -> {
          this.playbackState = PlaybackStateCompat.STATE_PAUSED
        }
      }
    }

    updatePlaybackState(this.playbackState)

    mediaSession.setMetadata(md.build())
    mediaSession.setActive(true)
    mediaNotificationManager.show(nb, isPlaying)
  }

  fun resetLockScreenInfo() {
    if (artworkThread != null && artworkThread!!.isAlive) artworkThread!!.interrupt()
    artworkThread = null

    mediaNotificationManager.hide()
    mediaSession.setActive(false)
  }

  fun enableRemoteCommand(
    name: String,
    enabled: Boolean,
  ) {
    var controlValue = 0L
    when (name) {
      "play" -> controlValue = PlaybackStateCompat.ACTION_PLAY
      "pause" -> controlValue = PlaybackStateCompat.ACTION_PAUSE
      "stop" -> controlValue = PlaybackStateCompat.ACTION_STOP
      "togglePlayPause" -> controlValue = PlaybackStateCompat.ACTION_PLAY_PAUSE
      "nextTrack" -> controlValue = PlaybackStateCompat.ACTION_SKIP_TO_NEXT
      "previousTrack" -> controlValue = PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS
      "skipForward" -> controlValue = PlaybackStateCompat.ACTION_REWIND
      "skipBackward" -> controlValue = PlaybackStateCompat.ACTION_REWIND
    }

    controls =
      if (enabled) {
        controls or controlValue
      } else {
        controls and controlValue.inv()
      }

    mediaNotificationManager.updateActions(controls)
    pb.setActions(controls)

    state = pb.build()
    mediaSession.setPlaybackState(state)

    updateNotificationMediaStyle()

    if (mediaSession.isActive) {
      mediaNotificationManager.show(nb, isPlaying)
    }
  }

  private fun loadArtwork(
    url: String,
    local: Boolean,
  ): Bitmap? {
    var bitmap: Bitmap? = null

    try {
      // If we are running the app in debug mode, the "local" image will be served from htt://localhost:8080, so we need to check for this case and load those images from URL
      if (local && !url.startsWith("http")) {
        // Gets the drawable from the RN's helper for local resources
        val helper = instance
        val image = helper.getResourceDrawable(reactContext, url)

        bitmap =
          if (image is BitmapDrawable) {
            image.bitmap
          } else {
            BitmapFactory.decodeFile(url)
          }
      } else {
        // Open connection to the URL and decodes the image
        val con = URL(url).openConnection()
        con.connect()
        val input = con.getInputStream()
        bitmap = BitmapFactory.decodeStream(input)
        input.close()
      }
    } catch (ex: IOException) {
      Log.w("MediaSessionManager", "Could not load the artwork", ex)
    } catch (ex: IndexOutOfBoundsException) {
      Log.w("MediaSessionManager", "Could not load the artwork", ex)
    }

    return bitmap
  }

  fun updatePlaybackState(playbackState: Int) {
    isPlaying = playbackState == PlaybackStateCompat.STATE_PLAYING

    pb.setState(playbackState, elapsedTime, speed)
    pb.setActions(controls)
    state = pb.build()
    mediaSession.setPlaybackState(state)
  }

  private fun hasControl(control: Long): Boolean = (controls and control) == control

  private fun updateNotificationMediaStyle() {
    val style = MediaStyle()
    style.setMediaSession(mediaSession.sessionToken)
    var controlCount = 0
    if (hasControl(PlaybackStateCompat.ACTION_PLAY) ||
      hasControl(PlaybackStateCompat.ACTION_PAUSE) ||
      hasControl(
        PlaybackStateCompat.ACTION_PLAY_PAUSE,
      )
    ) {
      controlCount += 1
    }
    if (hasControl(PlaybackStateCompat.ACTION_SKIP_TO_NEXT)) {
      controlCount += 1
    }
    if (hasControl(PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS)) {
      controlCount += 1
    }
    if (hasControl(PlaybackStateCompat.ACTION_FAST_FORWARD)) {
      controlCount += 1
    }
    if (hasControl(PlaybackStateCompat.ACTION_REWIND)) {
      controlCount += 1
    }
    val actions = IntArray(controlCount)
    for (i in actions.indices) {
      actions[i] = i
    }
    style.setShowActionsInCompactView(*actions)
    nb.setStyle(style)
  }
}

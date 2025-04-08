package com.swmansion.audioapi.system

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.drawable.BitmapDrawable
import android.os.Build
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.RatingCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import androidx.media.app.NotificationCompat.MediaStyle
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.bridge.ReadableType
import com.facebook.react.views.imagehelper.ResourceDrawableIdHelper.Companion.instance
import java.io.IOException
import java.net.URL

class MediaSessionManager(val reactContext: ReactApplicationContext) {
  private var mediaSession: MediaSessionCompat = MediaSessionCompat(reactContext, "MediaSessionManager")
  private var nb: NotificationCompat.Builder
  private var mediaNotificationManager: MediaNotificationManager

  private val controls: Long = 0
  private var ratingType: Int = RatingCompat.RATING_PERCENTAGE
  private var isPlaying: Boolean = false

  val notificationId = 100
  private val channelId = "react-native-audio-api"

  private var artworkThread: Thread? = null

  init {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      createChannel()
    }

    this.nb = NotificationCompat.Builder(reactContext, channelId)
    this.nb.setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
    this.nb.setPriority(NotificationCompat.PRIORITY_HIGH)

    updateNotificationMediaStyle()

    this.mediaNotificationManager = MediaNotificationManager(reactContext, notificationId)
  }

  private fun hasControl(control: Long): Boolean {
    return (controls and control) == control
  }

  private fun updateNotificationMediaStyle() {
    val style = MediaStyle()
    style.setMediaSession(mediaSession.sessionToken)
    var controlCount = 0
    if (hasControl(PlaybackStateCompat.ACTION_PLAY) || hasControl(PlaybackStateCompat.ACTION_PAUSE) || hasControl(
        PlaybackStateCompat.ACTION_PLAY_PAUSE
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

  @RequiresApi(Build.VERSION_CODES.O)
  private fun createChannel() {
    val notificationManager =
      reactContext.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

    val mChannel =
      NotificationChannel(channelId, "Audio manager", NotificationManager.IMPORTANCE_LOW)
    mChannel.description = "Audio manager"
    mChannel.setShowBadge(false)
    mChannel.lockscreenVisibility = NotificationCompat.VISIBILITY_PUBLIC
    notificationManager.createNotificationChannel(mChannel)
  }

  fun setLockScreenInfo(info: ReadableMap?) {
    if (artworkThread != null && artworkThread!!.isAlive) artworkThread!!.interrupt()
    artworkThread = null

    if (info == null) {
      return
    }

    val md = MediaMetadataCompat.Builder()

    val title: String? = if (info.hasKey("title")) info.getString("title") else null
    val artist: String? = if (info.hasKey("artist")) info.getString("artist") else null
    val album: String? = if (info.hasKey("album")) info.getString("album") else null
    val genre: String? = if (info.hasKey("genre")) info.getString("genre") else null
    val description: String? =
      if (info.hasKey("description")) info.getString("description") else null
    val date: String? = if (info.hasKey("date")) info.getString("date") else null
    val duration: Long =
      if (info.hasKey("duration")) (info.getDouble("duration") * 1000).toLong() else 0
    val notificationColor =
      if (info.hasKey("color")) info.getInt("color") else NotificationCompat.COLOR_DEFAULT
    val isColorized: Boolean =
      if (info.hasKey("colorized")) info.getBoolean("colorized") else !info.hasKey("color")
    val notificationIcon: String? =
      if (info.hasKey("notificationIcon")) info.getString("notificationIcon") else null

    val rating = if (info.hasKey("rating")) {
      if (ratingType == RatingCompat.RATING_PERCENTAGE) {
        RatingCompat.newPercentageRating(info.getDouble("rating").toFloat())
      } else if (ratingType == RatingCompat.RATING_HEART) {
        RatingCompat.newHeartRating(info.getBoolean("rating"))
      } else if (ratingType == RatingCompat.RATING_THUMB_UP_DOWN) {
        RatingCompat.newThumbRating(info.getBoolean("rating"))
      } else {
        RatingCompat.newStarRating(
          RatingCompat.RATING_3_STARS,
          info.getDouble("rating").toFloat()
        )
      }
    } else {
      RatingCompat.newUnratedRating(ratingType)
    }

    md.putText(MediaMetadataCompat.METADATA_KEY_TITLE, title)
    md.putText(MediaMetadataCompat.METADATA_KEY_ARTIST, artist)
    md.putText(MediaMetadataCompat.METADATA_KEY_ALBUM, album)
    md.putText(MediaMetadataCompat.METADATA_KEY_GENRE, genre)
    md.putText(MediaMetadataCompat.METADATA_KEY_DISPLAY_DESCRIPTION, description)
    md.putText(MediaMetadataCompat.METADATA_KEY_DATE, date)
    md.putLong(MediaMetadataCompat.METADATA_KEY_DURATION, duration)
    md.putRating(MediaMetadataCompat.METADATA_KEY_RATING, rating)

    nb.setContentTitle(title)
    nb.setContentText(artist)
    nb.setContentInfo(album)
    nb.setColor(notificationColor)
    nb.setColorized(isColorized)

    if (notificationIcon != null) {
      mediaNotificationManager.setCustomNotificationIcon(notificationIcon)
    }

    if (info.hasKey("artwork")) {
      val artwork: String?
      var localArtwork = false

      if (info.getType("artwork") == ReadableType.Map) {
        artwork = info.getMap("artwork")?.getString("uri")
        localArtwork = true
      } else {
        artwork = info.getString("artwork")
      }

      val artworkLocal = localArtwork

      artworkThread = Thread {
        try {
          val bitmap: Bitmap? = artwork?.let { loadArtwork(it, artworkLocal) }

          val currentMetadata: MediaMetadataCompat = mediaSession.controller.metadata
          val newBuilder =
            MediaMetadataCompat.Builder(
              currentMetadata
            )
          mediaSession.setMetadata(
            newBuilder.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, bitmap).build()
          )
          // If enabled, Android 8+ "colorizes" the notification color by extracting colors from the artwork
          nb.setColorized(isColorized)

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

    mediaSession.setMetadata(md.build())
    mediaSession.setActive(true)
    mediaNotificationManager.show(nb, isPlaying)
  }

  fun resetLockScreenInfo() {
    if(artworkThread != null && artworkThread!!.isAlive) artworkThread!!.interrupt();
    artworkThread = null;

    mediaNotificationManager.hide()
    mediaSession.setActive(false)
  }

  private fun loadArtwork(url: String, local: Boolean): Bitmap? {
    var bitmap: Bitmap? = null

    try {
      // If we are running the app in debug mode, the "local" image will be served from htt://localhost:8080, so we need to check for this case and load those images from URL
      if (local && !url.startsWith("http")) {
        // Gets the drawable from the RN's helper for local resources
        val helper = instance
        val image = helper.getResourceDrawable(reactContext, url)

        bitmap = if (image is BitmapDrawable) {
          (image as BitmapDrawable).bitmap
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
}

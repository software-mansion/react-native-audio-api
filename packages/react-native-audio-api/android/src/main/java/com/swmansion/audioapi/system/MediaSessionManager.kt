package com.swmansion.audioapi.system

import android.Manifest
import android.app.Activity
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build
import android.os.IBinder
import android.support.v4.media.session.MediaSessionCompat
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference

object MediaSessionManager {
  private lateinit var audioAPIModule: WeakReference<AudioAPIModule>
  private lateinit var reactContext: WeakReference<ReactApplicationContext>
  const val NOTIFICATION_ID = 100
  const val CHANNEL_ID = "react-native-audio-api"

  private lateinit var audioManager: AudioManager
  private lateinit var mediaSession: MediaSessionCompat
  lateinit var mediaNotificationManager: MediaNotificationManager
  private lateinit var lockScreenManager: LockScreenManager
  private lateinit var audioFocusListener: AudioFocusListener
  private lateinit var volumeChangeListener: VolumeChangeListener
  private lateinit var mediaReceiver: MediaReceiver

  private val connection =
    object : ServiceConnection {
      override fun onServiceConnected(
        name: ComponentName,
        service: IBinder,
      ) {
        Log.w("MediaSessionManager", "onServiceConnected")
        val binder = service as MediaNotificationManager.NotificationService.LocalBinder
        val notificationService = binder.getService()
        notificationService?.forceForeground()
        reactContext.get()?.unbindService(this)
      }

      override fun onServiceDisconnected(name: ComponentName) {
        Log.w("MediaSessionManager", "Service is disconnected.")
      }

      override fun onBindingDied(name: ComponentName) {
        Log.w("MediaSessionManager", "Binding has died.")
      }

      override fun onNullBinding(name: ComponentName) {
        Log.w("MediaSessionManager", "Bind was null.")
      }
    }

  fun initialize(
    audioAPIModule: WeakReference<AudioAPIModule>,
    reactContext: WeakReference<ReactApplicationContext>,
  ) {
    this.audioAPIModule = audioAPIModule
    this.reactContext = reactContext
    this.audioManager = reactContext.get()?.getSystemService(Context.AUDIO_SERVICE) as AudioManager
    this.mediaSession = MediaSessionCompat(reactContext.get()!!, "MediaSessionManager")

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      createChannel()
    }

    this.mediaNotificationManager = MediaNotificationManager(this.reactContext)
    this.lockScreenManager = LockScreenManager(this.reactContext, WeakReference(this.mediaSession), WeakReference(mediaNotificationManager))
    this.mediaReceiver =
      MediaReceiver(this.reactContext, WeakReference(this.mediaSession), WeakReference(mediaNotificationManager), this.audioAPIModule)
    this.mediaSession.setCallback(MediaSessionCallback(this.audioAPIModule, WeakReference(this.lockScreenManager)))

    val filter = IntentFilter()
    filter.addAction(MediaNotificationManager.REMOVE_NOTIFICATION)
    filter.addAction(MediaNotificationManager.MEDIA_BUTTON)
    filter.addAction(Intent.ACTION_MEDIA_BUTTON)
    filter.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY)

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      this.reactContext.get()!!.registerReceiver(mediaReceiver, filter, Context.RECEIVER_EXPORTED)
    } else {
      ContextCompat.registerReceiver(
        this.reactContext.get()!!,
        mediaReceiver,
        filter,
        ContextCompat.RECEIVER_NOT_EXPORTED,
      )
    }

    this.audioFocusListener =
      AudioFocusListener(WeakReference(this.audioManager), this.audioAPIModule, WeakReference(this.lockScreenManager))
    this.volumeChangeListener = VolumeChangeListener(WeakReference(this.audioManager), this.audioAPIModule)

    val myIntent = Intent(this.reactContext.get(), MediaNotificationManager.NotificationService::class.java)

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      try {
        this.reactContext.get()?.bindService(myIntent, connection, Context.BIND_AUTO_CREATE)
      } catch (ignored: Exception) {
        ContextCompat.startForegroundService(this.reactContext.get()!!, myIntent)
      }
    } else {
      this.reactContext.get()?.startService(myIntent)
    }
  }

  fun setLockScreenInfo(info: ReadableMap?) {
    lockScreenManager.setLockScreenInfo(info)
  }

  fun resetLockScreenInfo() {
    lockScreenManager.resetLockScreenInfo()
  }

  fun enableRemoteCommand(
    name: String,
    enabled: Boolean,
  ) {
    lockScreenManager.enableRemoteCommand(name, enabled)
  }

  fun getDevicePreferredSampleRate(): Double {
    val sampleRate = this.audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)
    return sampleRate.toDouble()
  }

  fun observeVolumeChanges(observe: Boolean) {
    if (observe) {
      ContextCompat.registerReceiver(
        reactContext.get()!!,
        volumeChangeListener,
        volumeChangeListener.getIntentFilter(),
        ContextCompat.RECEIVER_NOT_EXPORTED,
      )
    } else {
      reactContext.get()?.unregisterReceiver(volumeChangeListener)
    }
  }

  fun requestRecordingPermissions(currentActivity: Activity?): String {
    ActivityCompat.requestPermissions(
      currentActivity!!,
      arrayOf(Manifest.permission.RECORD_AUDIO),
      200,
    )
    return checkRecordingPermissions()
  }

  fun requestAudioFocus(request: ReadableMap) {
    val afbd: AudioFocusRequest.Builder = AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
    var focusGain: Int = AudioManager.AUDIOFOCUS_GAIN
    val legacyStreamType: Int = AudioManager.STREAM_MUSIC
    val aabd: AudioAttributes.Builder = AudioAttributes.Builder()
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      if (request.hasKey("focusGain")) {
        when (request.getString("focusGain")) {
          "audiofocus_gain" -> focusGain = AudioManager.AUDIOFOCUS_GAIN
          "audiofocus_gain_transient" -> focusGain = AudioManager.AUDIOFOCUS_GAIN_TRANSIENT
          "audiofocus_gain_transient_exclusive" ->
            focusGain =
              AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE

          "audiofocus_gain_transient_may_duck" ->
            focusGain =
              AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK
        }
      }
      if (request.hasKey("pauseWhenDucked")) {
        when (request.getBoolean("pauseWhenDucked")) {
          true -> afbd.setWillPauseWhenDucked(true)
          false -> afbd.setWillPauseWhenDucked(false)
        }
      }
      if (request.hasKey("acceptsDelayedFocusGain")) {
        when (request.getBoolean("acceptsDelayedFocusGain")) {
          true -> afbd.setAcceptsDelayedFocusGain(true)
          false -> afbd.setAcceptsDelayedFocusGain(false)
        }
      }
      if (request.hasKey("audioAttributes")) {
        val audioAttributesMap = request.getMap("audioAttributes")
        if (audioAttributesMap != null) {
          if (audioAttributesMap.hasKey("allowedCapturePolicy")) {
            when (audioAttributesMap.getString("allowedCapturePolicy")) {
              "allow_capture_by_all" -> aabd.setAllowedCapturePolicy(AudioAttributes.ALLOW_CAPTURE_BY_ALL)
              "allow_capture_by_system" -> aabd.setAllowedCapturePolicy(AudioAttributes.ALLOW_CAPTURE_BY_SYSTEM)
              "allow_capture_by_none" -> aabd.setAllowedCapturePolicy(AudioAttributes.ALLOW_CAPTURE_BY_NONE)
            }
          }
          if (audioAttributesMap.hasKey("contentType")) {
            when (audioAttributesMap.getString("contentType")) {
              "content_type_sonification" -> aabd.setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
              "content_type_music" -> aabd.setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
              "content_type_movie" -> aabd.setContentType(AudioAttributes.CONTENT_TYPE_MOVIE)
              "content_type_speech" -> aabd.setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
              "content_type_unknown" -> aabd.setContentType(AudioAttributes.CONTENT_TYPE_UNKNOWN)
            }
          }
          if (audioAttributesMap.hasKey("flag")) {
            when (audioAttributesMap.getString("flag")) {
              "flag_hw_av_sync" -> aabd.setFlags(AudioAttributes.FLAG_HW_AV_SYNC)
              "flag_audibility_enforced" -> aabd.setFlags(AudioAttributes.FLAG_AUDIBILITY_ENFORCED)
            }
          }
          if (audioAttributesMap.hasKey("hapticChannelsMuted")) {
            when (audioAttributesMap.getBoolean("hapticChannelsMuted")) {
              true -> aabd.setHapticChannelsMuted(true)
              false -> aabd.setHapticChannelsMuted(false)
            }
          }
          if (audioAttributesMap.hasKey("isContentSpatialized")) {
            when (audioAttributesMap.getBoolean("isContentSpatialized")) {
              true -> aabd.setIsContentSpatialized(true)
              false -> aabd.setIsContentSpatialized(false)
            }
          }
          if (audioAttributesMap.hasKey("legacyStreamType")) {
            when (audioAttributesMap.getString("legacyStreamType")) {
              "stream_voice_call" -> aabd.setLegacyStreamType(AudioManager.STREAM_VOICE_CALL)
              "stream_system" -> aabd.setLegacyStreamType(AudioManager.STREAM_SYSTEM)
              "stream_ring" -> aabd.setLegacyStreamType(AudioManager.STREAM_RING)
              "stream_music" -> aabd.setLegacyStreamType(AudioManager.STREAM_MUSIC)
              "stream_alarm" -> aabd.setLegacyStreamType(AudioManager.STREAM_ALARM)
              "stream_notification" -> aabd.setLegacyStreamType(AudioManager.STREAM_NOTIFICATION)
            }
          }
          if (audioAttributesMap.hasKey("spatializationBehavior")) {
          }
        }
        audioFocusListener.requestAudioFocus(afbd, aabd, legacyStreamType, focusGain)
      }
    }
  }

  fun abandonAudioFocus() {
    audioFocusListener.abandonAudioFocus()
  }

  fun checkRecordingPermissions(): String =
    if (ContextCompat.checkSelfPermission(
        reactContext.get()!!,
        Manifest.permission.RECORD_AUDIO,
      ) == PackageManager.PERMISSION_GRANTED
    ) {
      "Granted"
    } else {
      "Denied"
    }

  @RequiresApi(Build.VERSION_CODES.O)
  private fun createChannel() {
    val notificationManager =
      reactContext.get()?.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

    val mChannel =
      NotificationChannel(CHANNEL_ID, "Audio manager", NotificationManager.IMPORTANCE_LOW)
    mChannel.description = "Audio manager"
    mChannel.setShowBadge(false)
    mChannel.lockscreenVisibility = NotificationCompat.VISIBILITY_PUBLIC
    notificationManager.createNotificationChannel(mChannel)
  }
}

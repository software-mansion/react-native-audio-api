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

  fun parseAudioFocusOptionMap(request: ReadableMap): Map<String, Int> {
    val audioFocusOptions = HashMap<String, Int>()
    if (request.hasKey("focusGain")) {
      when (request.getString("focusGain")) {
        "audiofocus_gain" -> audioFocusOptions["focusGain"] = AudioManager.AUDIOFOCUS_GAIN
        "audiofocus_gain_transient" -> audioFocusOptions["focusGain"] = AudioManager.AUDIOFOCUS_GAIN_TRANSIENT
        "audiofocus_gain_transient_exclusive" ->
          audioFocusOptions["focusGain"] = AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE
        "audiofocus_gain_transient_may_duck" -> AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK
      }
    }
    if (request.hasKey("acceptsDelayedFocusGain")) {
      when (request.getBoolean("acceptsDelayedFocusGain")) {
        true -> audioFocusOptions["acceptsDelayedFocusGain"] = 1
        false -> audioFocusOptions["acceptsDelayedFocusGain"] = 0
      }
    }
    if (request.hasKey("pauseWhenDucked")) {
      audioFocusOptions["pauseWhenDucked"] = if (request.getBoolean("pauseWhenDucked")) 1 else 0
    }
    if (request.hasKey("audioAttributes")) {
      val values: ReadableMap? = request.getMap("audioAttributes")
      if (values?.hasKey("allowedCapturePolicy") == true) {
        when (values.getString("allowedCapturePolicy")) {
          "allow_capture_by_all" ->
            audioFocusOptions["allowedCapturePolicy"] =
              AudioAttributes.ALLOW_CAPTURE_BY_ALL

          "allow_capture_by_system" ->
            audioFocusOptions["allowedCapturePolicy"] =
              AudioAttributes.ALLOW_CAPTURE_BY_SYSTEM

          "allow_capture_by_none" ->
            audioFocusOptions["allowedCapturePolicy"] =
              AudioAttributes.ALLOW_CAPTURE_BY_NONE
        }
      }
      if (values?.hasKey("contentType") == true) {
        when (values.getString("contentType")) {
          "content_type_movie" ->
            audioFocusOptions["contentType"] =
              AudioAttributes.CONTENT_TYPE_MOVIE

          "content_type_music" ->
            audioFocusOptions["contentType"] =
              AudioAttributes.CONTENT_TYPE_MOVIE

          "content_type_music" ->
            audioFocusOptions["contentType"] =
              AudioAttributes.CONTENT_TYPE_MUSIC

          "content_type_speech" ->
            audioFocusOptions["contentType"] =
              AudioAttributes.CONTENT_TYPE_SPEECH

          "content_type_unknown" ->
            audioFocusOptions["contentType"] =
              AudioAttributes.CONTENT_TYPE_UNKNOWN
        }
      }
      if (values?.hasKey("flag") == true) {
        when (values.getString("flag")) {
          "flag_hw_av_sync" -> audioFocusOptions["flag"] = AudioAttributes.FLAG_HW_AV_SYNC
          "flag_audibility_enforced" ->
            audioFocusOptions["flag"] =
              AudioAttributes.FLAG_AUDIBILITY_ENFORCED
        }
      }
      if (values?.hasKey("hapticChannelsMuted") == true) {
        audioFocusOptions["hapticChannelsMuted"] = if (values.getBoolean("hapticChannelsMuted")) 1 else 0
      }
      if (values?.hasKey("isContentSpatialized") == true) {
        audioFocusOptions["isContentSpatialized"] = if (values.getBoolean("isContentSpatialized")) 1 else 0
      }
      if (values?.hasKey("spatializationBehavior") == true) {
        when (values.getString("spatializationBehavior")) {
          "spatialization_behavior_auto" ->
            audioFocusOptions["spatializationBehavior"] =
              AudioAttributes.SPATIALIZATION_BEHAVIOR_AUTO

          "spatialization_behavior_never" ->
            audioFocusOptions["spatializationBehavior"] =
              AudioAttributes.SPATIALIZATION_BEHAVIOR_NEVER
        }
      }
      if (values?.hasKey("usage") == true) {
        when (values.getString("usage")) {
          "usage_alarm" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_ALARM
          "usage_assistance_accessibility" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY
          "usage_assistance_navigation_guidance" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE
          "usage_assistance_sonification" ->
            audioFocusOptions["usage"] = AudioAttributes.USAGE_ASSISTANCE_SONIFICATION
          "usage_assistant" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_ASSISTANT
          "usage_game" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_GAME
          "usage_media" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_MEDIA
          "usage_notification" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_NOTIFICATION
          "usage_notification_event" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_NOTIFICATION_EVENT
          "usage_notification_ringtone" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_NOTIFICATION_RINGTONE
          "usage_notification_communication_request" ->
            audioFocusOptions["usage"] =
              AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_REQUEST
          "usage_notification_communication_instant" ->
            audioFocusOptions["usage"] =
              AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_INSTANT
          "usage_notification_communication_delayed" ->
            audioFocusOptions["usage"] =
              AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_DELAYED
          "usage_unknown" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_UNKNOWN
          "usage_voice_communication" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_VOICE_COMMUNICATION
          "usage_voice_communication_signalling" -> audioFocusOptions["usage"] = AudioAttributes.USAGE_VOICE_COMMUNICATION_SIGNALLING
        }
      }
    }

    return audioFocusOptions
  }

  @RequiresApi(Build.VERSION_CODES.O)
  fun requestAudioFocus(
    options: ReadableMap,
    observeAudioInterruptions: Boolean,
  ) {
    val parsedRequest = parseAudioFocusOptionMap(options)
    val afbd = AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
    val aabd = AudioAttributes.Builder()
    var pauseWhenDucked = false
    var acceptsDelayedFocusGain = false
    if (parsedRequest.containsKey("pauseWhenDucked")) {
      pauseWhenDucked = parsedRequest["pauseWhenDucked"] == 1
      afbd.setWillPauseWhenDucked(pauseWhenDucked)
    }
    parsedRequest["focusGain"]?.let { afbd.setFocusGain(it) }
    if (parsedRequest.containsKey("acceptsDelayedFocusGain")) {
      acceptsDelayedFocusGain = parsedRequest["acceptsDelayedFocusGain"] == 1
      afbd.setAcceptsDelayedFocusGain(acceptsDelayedFocusGain)
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      parsedRequest["allowedCapturePolicy"]?.let { aabd.setAllowedCapturePolicy(it) }
      parsedRequest["contentType"]?.let { aabd.setAllowedCapturePolicy(it) }
      if (parsedRequest.containsKey("hapticChannelsMuted")) {
        aabd.setHapticChannelsMuted(parsedRequest["hapticChannelsMuted"] == 1)
      }
    }
    parsedRequest["flag"]?.let { aabd.setFlags(it) }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S_V2) {
      parsedRequest["spatializationBehavior"]?.let { aabd.setSpatializationBehavior(it) }
      if (parsedRequest.containsKey("isContentSpatialized")) {
        aabd.setIsContentSpatialized(parsedRequest["isContentSpatialized"] == 1)
      }
    }
    parsedRequest["usage"]?.let { aabd.setUsage(it) }
    afbd.setAudioAttributes(aabd.build())
    // according to docs: OnAudioFocusChangeListener is only required
    // if you also specify willPauseWhenDucked(true) or setAcceptsDelayedFocusGain(true) in the request.
    if ((pauseWhenDucked || acceptsDelayedFocusGain) && !observeAudioInterruptions) {
      throw IllegalArgumentException(
        "observeAudioInterruptions must be true when pauseWhenDucked or acceptsDelayedFocusGain is set to true",
      )
    }
    audioFocusListener.requestAudioFocus(afbd, observeAudioInterruptions)
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

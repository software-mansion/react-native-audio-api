package com.swmansion.audioapi.system

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.ServiceConnection
import android.media.AudioManager
import android.os.Build
import android.os.IBinder
import android.support.v4.media.session.MediaSessionCompat
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap

class MediaSessionManager(
  val reactContext: ReactApplicationContext,
) {
  val notificationId = 100
  val channelId = "react-native-audio-api"

  private val audioManager: AudioManager = reactContext.getSystemService(Context.AUDIO_SERVICE) as AudioManager
  val mediaSession: MediaSessionCompat = MediaSessionCompat(reactContext, "MediaSessionManager")
  val mediaNotificationManager: MediaNotificationManager
  private val lockScreenManager: LockScreenManager
  val eventEmitter: MediaSessionEventEmitter =
    MediaSessionEventEmitter(reactContext, notificationId)
  private val audioFocusListener: AudioFocusListener
  private val mediaReceiver: MediaReceiver =
    MediaReceiver(reactContext, this)

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
        reactContext.unbindService(this)
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

  init {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      createChannel()
    }

    this.mediaNotificationManager = MediaNotificationManager(reactContext, notificationId, channelId)
    this.lockScreenManager = LockScreenManager(reactContext, mediaSession, mediaNotificationManager, channelId)
    this.mediaSession.setCallback(MediaSessionCallback(eventEmitter, lockScreenManager))

    val filter = IntentFilter()
    filter.addAction(MediaNotificationManager.REMOVE_NOTIFICATION)
    filter.addAction(MediaNotificationManager.MEDIA_BUTTON)
    filter.addAction(Intent.ACTION_MEDIA_BUTTON)
    filter.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY)

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      reactContext.registerReceiver(mediaReceiver, filter, Context.RECEIVER_EXPORTED)
    } else {
      ContextCompat.registerReceiver(
        reactContext,
        mediaReceiver,
        filter,
        ContextCompat.RECEIVER_NOT_EXPORTED,
      )
    }

    this.audioFocusListener = AudioFocusListener(audioManager, eventEmitter, lockScreenManager)

    val myIntent = Intent(reactContext, MediaNotificationManager.NotificationService::class.java)

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      try {
        reactContext.bindService(myIntent, connection, Context.BIND_AUTO_CREATE)
      } catch (ignored: Exception) {
        ContextCompat.startForegroundService(reactContext, myIntent)
      }
    } else {
      reactContext.startService(myIntent)
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

  fun observeAudioInterruptions(observe: Boolean) {
    if (observe) {
      audioFocusListener.requestAudioFocus()
    } else {
      audioFocusListener.abandonAudioFocus()
    }
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
}

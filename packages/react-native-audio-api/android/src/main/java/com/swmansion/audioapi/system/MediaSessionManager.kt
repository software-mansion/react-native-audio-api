package com.swmansion.audioapi.system

import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Build
import android.support.v4.media.session.MediaSessionCompat
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.modules.core.PermissionAwareActivity
import com.facebook.react.modules.core.PermissionListener
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.core.NativeAudioPlayer
import com.swmansion.audioapi.core.NativeAudioRecorder
import com.swmansion.audioapi.system.PermissionRequestListener.Companion.RECORDING_REQUEST_CODE
import java.lang.ref.WeakReference
import java.util.UUID

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

  private var isServiceRunning = false
  private val serviceStateLock = Any()
  private val nativeAudioPlayers = mutableMapOf<String, NativeAudioPlayer>()
  private val nativeAudioRecorders = mutableMapOf<String, NativeAudioRecorder>()

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
      MediaReceiver(this.reactContext, WeakReference(this.mediaSession), WeakReference(this.mediaNotificationManager), this.audioAPIModule)
    this.mediaSession.setCallback(MediaSessionCallback(this.audioAPIModule, WeakReference(this.mediaNotificationManager)))

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
  }

  fun attachAudioPlayer(player: NativeAudioPlayer): String {
    val uuid = UUID.randomUUID().toString()
    nativeAudioPlayers[uuid] = player

    return uuid
  }

  fun detachAudioPlayer(uuid: String) {
    nativeAudioPlayers.remove(uuid)
  }

  fun attachAudioRecorder(recorder: NativeAudioRecorder): String {
    val uuid = UUID.randomUUID().toString()
    nativeAudioRecorders[uuid] = recorder

    return uuid
  }

  fun detachAudioRecorder(uuid: String) {
    nativeAudioRecorders.remove(uuid)
  }

  fun startForegroundServiceIfNecessary() {
    if (nativeAudioPlayers.isNotEmpty() || nativeAudioRecorders.isNotEmpty()) {
      startForegroundService()
    }
  }

  fun stopForegroundServiceIfNecessary() {
    if (nativeAudioPlayers.isEmpty() && nativeAudioRecorders.isEmpty()) {
      stopForegroundService()
    }
  }

  private fun startForegroundService() {
    synchronized(serviceStateLock) {
      if (isServiceRunning || reactContext.get() == null) {
        return
      }

      val intent = Intent(reactContext.get(), MediaNotificationManager.AudioForegroundService::class.java)
      intent.action = MediaNotificationManager.ForegroundAction.START_FOREGROUND.name

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        ContextCompat.startForegroundService(reactContext.get()!!, intent)
      } else {
        reactContext.get()!!.startService(intent)
      }
      isServiceRunning = true
    }
  }

  private fun stopForegroundService() {
    synchronized(serviceStateLock) {
      if (!isServiceRunning || reactContext.get() == null) {
        return
      }

      val intent = Intent(reactContext.get(), MediaNotificationManager.AudioForegroundService::class.java)
      intent.action = MediaNotificationManager.ForegroundAction.STOP_FOREGROUND.name
      reactContext.get()!!.startService(intent)
      isServiceRunning = false
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

  fun activelyReclaimSession(enabled: Boolean) {
    // do nothing on android
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

  fun requestRecordingPermissions(permissionListener: PermissionListener) {
    val permissionAwareActivity = reactContext.get()!!.currentActivity as PermissionAwareActivity
    permissionAwareActivity.requestPermissions(arrayOf(Manifest.permission.RECORD_AUDIO), RECORDING_REQUEST_CODE, permissionListener)
  }

  fun checkRecordingPermissions(): String =
    if (reactContext.get()!!.checkSelfPermission(
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

  @RequiresApi(Build.VERSION_CODES.O)
  fun getDevicesInfo(): ReadableMap {
    val availableInputs = Arguments.createArray()
    val availableOutputs = Arguments.createArray()

    for (inputDevice in this.audioManager.getDevices(AudioManager.GET_DEVICES_INPUTS)) {
      val deviceInfo = Arguments.createMap()
      deviceInfo.putString("name", inputDevice.productName.toString())
      deviceInfo.putString("type", parseDeviceType(inputDevice))

      availableInputs.pushMap(deviceInfo)
    }

    for (outputDevice in this.audioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS)) {
      val deviceInfo = Arguments.createMap()
      deviceInfo.putString("name", outputDevice.productName.toString())
      deviceInfo.putString("type", parseDeviceType(outputDevice))

      availableOutputs.pushMap(deviceInfo)
    }

    val devicesInfo = Arguments.createMap()

    devicesInfo.putArray("currentInputs", Arguments.createArray())
    devicesInfo.putArray("currentOutputs", Arguments.createArray())
    devicesInfo.putArray("availableInputs", availableInputs)
    devicesInfo.putArray("availableOutputs", availableOutputs)

    return devicesInfo
  }

  @RequiresApi(Build.VERSION_CODES.O)
  fun parseDeviceType(device: AudioDeviceInfo): String =
    when (device.type) {
      AudioDeviceInfo.TYPE_BUILTIN_MIC -> "Built-in Mic"
      AudioDeviceInfo.TYPE_BUILTIN_EARPIECE -> "Built-in Earpiece"
      AudioDeviceInfo.TYPE_BUILTIN_SPEAKER -> "Built-in Speaker"
      AudioDeviceInfo.TYPE_WIRED_HEADSET -> "Wired Headset"
      AudioDeviceInfo.TYPE_WIRED_HEADPHONES -> "Wired Headphones"
      AudioDeviceInfo.TYPE_BLUETOOTH_A2DP -> "Bluetooth A2DP"
      AudioDeviceInfo.TYPE_BLUETOOTH_SCO -> "Bluetooth SCO"
      else -> "Other (${device.type})"
    }
}

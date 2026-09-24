package com.swmansion.audioapi.system

import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.annotation.RequiresPermission
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.modules.core.PermissionAwareActivity
import com.facebook.react.modules.core.PermissionListener
import com.swmansion.audioapi.AudioAPIModule
import com.swmansion.audioapi.system.PermissionRequestListener.Companion.RECORDING_REQUEST_CODE
import com.swmansion.audioapi.system.notification.NotificationRegistry
import com.swmansion.audioapi.system.notification.PlaybackNotification
import com.swmansion.audioapi.system.notification.PlaybackNotificationReceiver
import com.swmansion.audioapi.system.notification.RecordingNotification
import java.lang.ref.WeakReference

object MediaSessionManager {
  private lateinit var audioAPIModule: WeakReference<AudioAPIModule>
  private lateinit var reactContext: WeakReference<ReactApplicationContext>
  const val CHANNEL_ID = "react-native-audio-api"

  private lateinit var audioManager: AudioManager
  private lateinit var audioFocusListener: AudioFocusListener
  private lateinit var volumeChangeListener: VolumeChangeListener
  private lateinit var playbackNotificationReceiver: PlaybackNotificationReceiver

  private val mainHandler = Handler(Looper.getMainLooper())
  private var androidMode: String? = null
  private var configuredCommunicationDevice: String? = null
  private var previousAudioMode: Int? = null
  private var communicationSessionActive = false

  // The API-31 helper is intentionally opaque here so API-30 startup does not
  // need to resolve its Android-12-only callback types.
  private var communicationDeviceCallbacks: Any? = null

  // New notification system
  private lateinit var notificationRegistry: NotificationRegistry

  fun initialize(
    audioAPIModule: WeakReference<AudioAPIModule>,
    reactContext: WeakReference<ReactApplicationContext>,
  ) {
    this.audioAPIModule = audioAPIModule
    this.reactContext = reactContext
    this.audioManager = reactContext.get()?.getSystemService(Context.AUDIO_SERVICE) as AudioManager

    // Initialize ForegroundServiceManager
    ForegroundServiceManager.initialize(reactContext)

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      createChannel()
    }

    // Set up PlaybackNotificationReceiver
    PlaybackNotificationReceiver.setAudioAPIModule(audioAPIModule.get())
    this.playbackNotificationReceiver = PlaybackNotificationReceiver()

    // Register PlaybackNotificationReceiver
    val playbackFilter = IntentFilter(PlaybackNotificationReceiver.ACTION_NOTIFICATION_DISMISSED)
    playbackFilter.addAction(PlaybackNotification.MEDIA_BUTTON)
    playbackFilter.addAction(PlaybackNotificationReceiver.ACTION_SKIP_FORWARD)
    playbackFilter.addAction(PlaybackNotificationReceiver.ACTION_SKIP_BACKWARD)

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      this.reactContext.get()!!.registerReceiver(playbackNotificationReceiver, playbackFilter, Context.RECEIVER_NOT_EXPORTED)
    } else {
      ContextCompat.registerReceiver(
        this.reactContext.get()!!,
        playbackNotificationReceiver,
        playbackFilter,
        ContextCompat.RECEIVER_NOT_EXPORTED,
      )
    }

    this.audioFocusListener =
      AudioFocusListener(WeakReference(this.audioManager), this.audioAPIModule, mainHandler) {
        communicationSessionActive
      }
    this.volumeChangeListener = VolumeChangeListener(WeakReference(this.audioManager), this.audioAPIModule)

    // Initialize new notification system
    this.notificationRegistry = NotificationRegistry(this.reactContext, this.audioAPIModule)
  }

  /** The system output volume on the volumeChange event's own scale:
   * STREAM_MUSIC volume as a 0..1 fraction of its maximum. */
  fun getSystemVolume(): Double {
    val current = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC).toDouble()
    val max = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC).toDouble()
    return if (max > 0) current / max else 0.0
  }

  fun getDevicePreferredSampleRate(): Double {
    val sampleRate = this.audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)
    return sampleRate.toDouble()
  }

  fun setAudioSessionOptions(
    androidMode: String?,
    androidCommunicationDevice: String?,
  ) {
    runOnMain {
      this.androidMode = androidMode?.takeIf { it.isNotEmpty() }
      this.configuredCommunicationDevice = androidCommunicationDevice?.takeIf { it.isNotEmpty() }
    }
  }

  fun setAudioSessionActivity(
    enabled: Boolean,
    onComplete: (String?) -> Unit,
  ) {
    runOnMain {
      onComplete(setAudioSessionActivityOnMain(enabled))
    }
  }

  fun observeAudioInterruptions(
    focus: Int,
    enabled: Boolean,
  ) {
    runOnMain {
      if (communicationSessionActive) {
        return@runOnMain
      }
      if (enabled) {
        audioFocusListener.requestAudioFocus(focus)
      } else {
        audioFocusListener.abandonAudioFocus()
      }
    }
  }

  fun setCommunicationDevice(
    device: String,
    onComplete: (String?) -> Unit,
  ) {
    runOnMain {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
        onComplete("Android communication devices require Android 12 (API 31) or later")
        return@runOnMain
      }
      if (!communicationSessionActive) {
        onComplete("An Android communication session is not active")
        return@runOnMain
      }
      onComplete(requestCommunicationDevice(device))
    }
  }

  fun getCommunicationDevice(onComplete: (ReadableMap?) -> Unit) {
    runOnMain {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
        onComplete(null)
        return@runOnMain
      }
      onComplete(audioManager.communicationDevice?.let(::toReadableDeviceInfo))
    }
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun activateCommunicationSession(): String? {
    if (communicationSessionActive) {
      return null
    }

    previousAudioMode = audioManager.mode
    if (!audioFocusListener.requestCommunicationAudioFocus()) {
      previousAudioMode = null
      return "Android denied voice-communication audio focus"
    }

    return try {
      audioManager.mode = AudioManager.MODE_IN_COMMUNICATION
      communicationDeviceCallbacks =
        registerCommunicationDeviceCallbacks(audioManager, mainHandler, ::emitRouteChange)
      configuredCommunicationDevice?.let { device ->
        requestCommunicationDevice(device)?.let { error -> throw IllegalStateException(error) }
      }
      communicationSessionActive = true
      null
    } catch (error: Exception) {
      deactivateCommunicationSession()
      error.message ?: "Could not activate Android communication audio session"
    }
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun deactivateCommunicationSession() {
    if (!communicationSessionActive && previousAudioMode == null) {
      return
    }

    communicationSessionActive = false
    val callbacks = communicationDeviceCallbacks
    communicationDeviceCallbacks = null
    if (callbacks != null) {
      try {
        unregisterCommunicationDeviceCallbacks(callbacks)
      } catch (error: Exception) {
        Log.w("MediaSessionManager", "Could not unregister communication-device callbacks", error)
      }
    }
    audioManager.clearCommunicationDevice()
    audioFocusListener.abandonCommunicationAudioFocus()
    previousAudioMode?.let { audioManager.mode = it }
    previousAudioMode = null
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun requestCommunicationDevice(device: String): String? {
    if (device == "systemDefault") {
      audioManager.clearCommunicationDevice()
      return null
    }

    val deviceType =
      when (device) {
        "speaker" -> AudioDeviceInfo.TYPE_BUILTIN_SPEAKER
        "earpiece" -> AudioDeviceInfo.TYPE_BUILTIN_EARPIECE
        else -> return "Unsupported communication device: $device"
      }
    val target =
      audioManager.availableCommunicationDevices.firstOrNull { it.type == deviceType }
        ?: return "Requested communication device is unavailable: $device"

    if (!audioManager.setCommunicationDevice(target)) {
      return "Android rejected requested communication device: $device"
    }
    return null
  }

  private fun setAudioSessionActivityOnMain(enabled: Boolean): String? {
    if (!enabled) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        deactivateCommunicationSession()
      }
      return null
    }
    if (androidMode != "inCommunication") {
      return null
    }
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
      return "Android communication sessions require Android 12 (API 31) or later"
    }
    return activateCommunicationSession()
  }

  fun cleanup() {
    runOnMain {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        deactivateCommunicationSession()
      }
    }
  }

  private fun emitRouteChange(reason: String) {
    if (communicationSessionActive) {
      audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(
        AudioEvent.ROUTE_CHANGE.ordinal,
        mapOf("reason" to reason),
      )
    }
  }

  private fun runOnMain(action: () -> Unit) {
    if (Looper.myLooper() == mainHandler.looper) {
      action()
    } else {
      mainHandler.post(action)
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

  fun checkRecordingPermissions(): String {
    val context = reactContext.get()!!

    if (context.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) {
      return "Granted"
    }

    // Permission not granted - check if we should show rationale
    val activity = context.currentActivity
    if (activity != null &&
      ActivityCompat.shouldShowRequestPermissionRationale(
        activity,
        Manifest.permission.RECORD_AUDIO,
      )
    ) {
      // User previously denied but didn't select "Don't ask again"
      return "Denied"
    }

    // Either never asked OR user selected "Don't ask again"
    // Return "Undetermined" to match iOS behavior and let caller decide to request
    return "Undetermined"
  }

  fun requestNotificationPermissions(permissionListener: PermissionListener) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      val permissionAwareActivity = reactContext.get()!!.currentActivity as PermissionAwareActivity
      permissionAwareActivity.requestPermissions(
        arrayOf(Manifest.permission.POST_NOTIFICATIONS),
        PermissionRequestListener.NOTIFICATION_REQUEST_CODE,
        permissionListener,
      )
    } else {
      // For Android < 13, permission is granted by default
      val result = Arguments.createMap()
      result.putString("status", "Granted")
      permissionListener.onRequestPermissionsResult(
        PermissionRequestListener.NOTIFICATION_REQUEST_CODE,
        arrayOf(Manifest.permission.POST_NOTIFICATIONS),
        intArrayOf(PackageManager.PERMISSION_GRANTED),
      )
    }
  }

  fun checkNotificationPermissions(): String {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      val context = reactContext.get()!!

      if (context.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED) {
        return "Granted"
      }

      // Permission not granted - check if we should show rationale
      val activity = context.currentActivity
      if (activity != null &&
        ActivityCompat.shouldShowRequestPermissionRationale(
          activity,
          Manifest.permission.POST_NOTIFICATIONS,
        )
      ) {
        // User previously denied but didn't select "Don't ask again"
        return "Denied"
      }

      // Either never asked OR user selected "Don't ask again"
      return "Undetermined"
    }
    // For Android < 13, permission is granted by default
    return "Granted"
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
      deviceInfo.putString("id", inputDevice.getId().toString())
      deviceInfo.putString("name", inputDevice.productName.toString())
      deviceInfo.putString("category", parseDeviceCategory(inputDevice))

      availableInputs.pushMap(deviceInfo)
    }

    for (outputDevice in this.audioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS)) {
      val deviceInfo = Arguments.createMap()
      deviceInfo.putString("id", outputDevice.getId().toString())
      deviceInfo.putString("name", outputDevice.productName.toString())
      deviceInfo.putString("category", parseDeviceCategory(outputDevice))

      availableOutputs.pushMap(deviceInfo)
    }

    val devicesInfo = Arguments.createMap()

    devicesInfo.putArray("currentInputs", Arguments.createArray())
    devicesInfo.putArray("currentOutputs", Arguments.createArray())
    devicesInfo.putArray("availableInputs", availableInputs)
    devicesInfo.putArray("availableOutputs", availableOutputs)

    return devicesInfo
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun toReadableDeviceInfo(device: AudioDeviceInfo): ReadableMap =
    Arguments.createMap().apply {
      putString("id", device.id.toString())
      putString("name", device.productName.toString())
      putString("category", parseDeviceCategory(device))
    }

  @RequiresApi(Build.VERSION_CODES.O)
  fun parseDeviceCategory(device: AudioDeviceInfo): String =
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

  // Notification system methods
  @RequiresPermission(Manifest.permission.POST_NOTIFICATIONS)
  fun showNotification(
    type: String,
    key: String,
    options: ReadableMap?,
  ) {
    notificationRegistry.showNotification(key, type, options)
  }

  fun hideNotification(key: String) {
    notificationRegistry.hideNotification(key)
  }

  /**
   * Used by the  notification stop action, which also
   * unwinds the foreground service through the registry's unsubscribe path.
   */
  fun hideRecordingNotification() {
    if (!::notificationRegistry.isInitialized) {
      return
    }
    notificationRegistry.hideNotification(RecordingNotification.ID)
  }

  /**
   * Used by native-initiated pause/resume, which can't go through [showNotification] — there
   * is no JS to supply options.
   */
  fun setRecordingNotificationPaused(paused: Boolean) {
    if (!::notificationRegistry.isInitialized) {
      return
    }
    notificationRegistry.updateRecordingNotificationPausedState(paused)
  }

  fun isNotificationActive(key: String): Boolean = notificationRegistry.isNotificationActive(key)
}

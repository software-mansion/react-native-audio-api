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
import com.facebook.react.bridge.WritableMap
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
  private const val TAG = "MediaSessionManager"
  const val CHANNEL_ID = "react-native-audio-api"

  private lateinit var audioManager: AudioManager
  private lateinit var audioFocusListener: AudioFocusListener
  private lateinit var volumeChangeListener: VolumeChangeListener
  private lateinit var deviceChangeListener: DeviceChangeListener
  private lateinit var playbackNotificationReceiver: PlaybackNotificationReceiver

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
      AudioFocusListener(WeakReference(this.audioManager), this.audioAPIModule)
    this.volumeChangeListener = VolumeChangeListener(WeakReference(this.audioManager), this.audioAPIModule)
    this.deviceChangeListener = DeviceChangeListener(this.audioAPIModule)
    this.audioManager.registerAudioDeviceCallback(deviceChangeListener, Handler(Looper.getMainLooper()))

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

  fun cleanup() {
    if (::deviceChangeListener.isInitialized) {
      audioManager.unregisterAudioDeviceCallback(deviceChangeListener)
    }
  }

  fun requestAudioFocus(focus: Int) {
    audioFocusListener.requestAudioFocus(focus)
  }

  fun abandonAudioFocus() {
    audioFocusListener.abandonAudioFocus()
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

  /**
   * Capture device selected through `AudioAPIModule.setInputDevice`, reported
   * back by [getDevicesInfo]. Mirrors the selection held natively by
   * `AudioInputSelection`.
   *
   * Null means the platform picks the device. Android does not say which one
   * before a stream opens, so `currentInputs` stays empty.
   *
   * Written on the React Native module thread, read by any `getDevicesInfo` caller.
   */
  @Volatile
  private var preferredInputDeviceId: Int? = null

  @RequiresApi(Build.VERSION_CODES.M)
  fun findInputDevice(deviceId: String): AudioDeviceInfo? = recordableInputs().firstOrNull { it.id.toString() == deviceId }

  @RequiresApi(Build.VERSION_CODES.M)
  private fun recordableInputs(): List<AudioDeviceInfo> =
    this.audioManager
      .getDevices(AudioManager.GET_DEVICES_INPUTS)
      .filter { it.type !in systemOnlyInputTypes }

  private val systemOnlyInputTypes =
    setOf(
      AudioDeviceInfo.TYPE_TELEPHONY,
      AudioDeviceInfo.TYPE_REMOTE_SUBMIX,
      AudioDeviceInfo.TYPE_FM_TUNER,
      AudioDeviceInfo.TYPE_TV_TUNER,
    )

  /** Whether this manager started the Bluetooth SCO link, so it only tears down its own. */
  private var startedBluetoothSco = false

  @RequiresApi(Build.VERSION_CODES.M)
  fun setPreferredInputDevice(device: AudioDeviceInfo) {
    this.preferredInputDeviceId = device.id

    if (device.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO) {
      startBluetoothSco(device)
    } else {
      stopBluetoothSco()
    }
  }

  /**
   * A Bluetooth headset mic carries audio only over the SCO link, and the platform
   * brings that link up for the communication device, never for a capture stream's
   * preferred device. Without it the stream opens on the headset and receives no frames.
   * Output to the same headset moves from A2DP to SCO while the link is up.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  private fun startBluetoothSco(input: AudioDeviceInfo) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      val scoOutputs =
        audioManager.availableCommunicationDevices.filter {
          it.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO
        }
      val headset = scoOutputs.firstOrNull { it.address == input.address } ?: scoOutputs.firstOrNull()

      if (headset == null || !audioManager.setCommunicationDevice(headset)) {
        Log.w(TAG, "Cannot route communication to the Bluetooth headset, its mic stays silent")
        return
      }
    } else {
      @Suppress("DEPRECATION")
      audioManager.startBluetoothSco()
      @Suppress("DEPRECATION")
      audioManager.isBluetoothScoOn = true
    }

    startedBluetoothSco = true
  }

  private fun stopBluetoothSco() {
    if (!startedBluetoothSco) {
      return
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      audioManager.clearCommunicationDevice()
    } else {
      @Suppress("DEPRECATION")
      audioManager.isBluetoothScoOn = false
      @Suppress("DEPRECATION")
      audioManager.stopBluetoothSco()
    }

    startedBluetoothSco = false
  }

  @RequiresApi(Build.VERSION_CODES.O)
  fun getDevicesInfo(): ReadableMap {
    val availableInputs = Arguments.createArray()
    val currentInputs = Arguments.createArray()
    val availableOutputs = Arguments.createArray()

    val selectedInputDeviceId = this.preferredInputDeviceId

    for (inputDevice in recordableInputs()) {
      availableInputs.pushMap(describeDevice(inputDevice))

      if (inputDevice.id == selectedInputDeviceId) {
        currentInputs.pushMap(describeDevice(inputDevice))
      }
    }

    for (outputDevice in this.audioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS)) {
      availableOutputs.pushMap(describeDevice(outputDevice))
    }

    val devicesInfo = Arguments.createMap()

    devicesInfo.putArray("currentInputs", currentInputs)
    devicesInfo.putArray("currentOutputs", Arguments.createArray())
    devicesInfo.putArray("availableInputs", availableInputs)
    devicesInfo.putArray("availableOutputs", availableOutputs)

    return devicesInfo
  }

  @RequiresApi(Build.VERSION_CODES.O)
  private fun describeDevice(device: AudioDeviceInfo): WritableMap {
    val deviceInfo = Arguments.createMap()
    deviceInfo.putString("id", device.id.toString())
    deviceInfo.putString("name", deviceName(device))
    deviceInfo.putString("category", parseDeviceCategory(device))

    return deviceInfo
  }

  @RequiresApi(Build.VERSION_CODES.O)
  private fun deviceName(device: AudioDeviceInfo): String {
    val productName = device.productName.toString()

    if (device.type != AudioDeviceInfo.TYPE_BUILTIN_MIC || device.address.isEmpty()) {
      return productName
    }

    return "$productName (${device.address})"
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
      AudioDeviceInfo.TYPE_USB_DEVICE -> "USB Device"
      AudioDeviceInfo.TYPE_USB_HEADSET -> "USB Headset"
      AudioDeviceInfo.TYPE_USB_ACCESSORY -> "USB Accessory"
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

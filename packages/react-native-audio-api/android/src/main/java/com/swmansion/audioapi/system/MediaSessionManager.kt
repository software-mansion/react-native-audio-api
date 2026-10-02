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
import android.os.SystemClock
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
  private const val BLUETOOTH_SCO_CONNECT_TIMEOUT_MS = 3000L
  private const val BLUETOOTH_SCO_POLL_INTERVAL_MS = 50L
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

    if (::audioManager.isInitialized) {
      stopBluetoothSco()
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

  /** Guards [preferredInputDeviceId] and [acquiredInputRouteCount]. */
  private val inputSelectionLock = Any()

  /**
   * Null means the platform picks the capture device.
   *
   * Written under [inputSelectionLock]; volatile so [getDevicesInfo] can read it without.
   */
  @Volatile
  private var preferredInputDeviceId: Int? = null

  /**
   * Routes acquired and not yet released. Only one recording runs at a time, but a restart
   * can acquire its new route while a recording started in the meantime holds one too, so
   * this is a count and not a flag. Guarded by [inputSelectionLock].
   */
  private var acquiredInputRouteCount = 0

  /** The device the latest [acquireInputRoute] handed out, null for the system default. */
  @Volatile
  private var captureDeviceId: Int? = null

  /** What [acquireInputRoute] hands a recorder. */
  class InputRoute(
    /** Device to open the capture stream on, or null for the system default. */
    val deviceId: Int?,
    /** False when the routing that device needs could not be set up. */
    val routed: Boolean,
  )

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

  /**
   * Whether this manager started the Bluetooth SCO link, so it only tears down its own.
   * Volatile: recorder threads and the main thread both read and write it.
   */
  @Volatile
  private var startedBluetoothSco = false

  /**
   * A running or paused recording is moved onto the new choice before this returns, which
   * blocks while its stream reopens; never call it on the main thread.
   *
   * @param device null hands the choice back to the system.
   * @return false when a recording was running and could not continue on the new input.
   */
  fun setPreferredInputDevice(device: AudioDeviceInfo?): Boolean {
    synchronized(inputSelectionLock) {
      if (device?.id == preferredInputDeviceId) {
        return true
      }

      preferredInputDeviceId = device?.id
    }

    // Does nothing when no recording is in progress.
    return NativeInputRouting.rerouteActiveCapture()
  }

  /**
   * Moves a recording in progress onto an external mic that just connected, when the input
   * is left to the system and that mic is now the one [automaticInput] prefers.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  fun followAddedInputDevices(addedDevices: Array<out AudioDeviceInfo>) {
    val followsSystemInput = preferredInputDeviceId == null

    if (!followsSystemInput || addedDevices.none { it.isSource && it.type in automaticInputTypes }) {
      return
    }

    val preferredInput = automaticInput()

    if (preferredInput == null || preferredInput.id == captureDeviceId) {
      return
    }

    // Off the calling (main) thread: rerouting can wait for a Bluetooth headset's link.
    // Does nothing when no recording is in progress.
    Thread { NativeInputRouting.rerouteActiveCapture() }.start()
  }

  /**
   * Tells a recorder about to open its capture stream which device to open it on, after
   * bringing up the routing that device needs and waiting for it.
   *
   * Blocks for up to [BLUETOOTH_SCO_CONNECT_TIMEOUT_MS], so never call it on the main thread.
   * Every call must be balanced by [releaseInputRoute], also when the route reports `routed = false`.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  fun acquireInputRoute(): InputRoute {
    val selectedInputDeviceId =
      synchronized(inputSelectionLock) {
        acquiredInputRouteCount++
        preferredInputDeviceId
      }

    // Outside the lock, so the wait cannot stall the device-removal callback on the main thread.
    if (selectedInputDeviceId != null) {
      val selectedInput = recordableInputs().firstOrNull { it.id == selectedInputDeviceId }
      val routed = selectedInput == null || prepareCaptureRouting(selectedInput)
      captureDeviceId = selectedInputDeviceId
      return InputRoute(selectedInputDeviceId, routed)
    }

    captureDeviceId = automaticInputId()
    return InputRoute(captureDeviceId, routed = true)
  }

  /**
   * External mics a recording uses when nothing is selected, most preferred first. A
   * dedicated USB mic or interface is the clearest sign of intent; a Bluetooth headset
   * comes last because its mic is call quality and costs the headset its playback quality.
   */
  private val automaticInputTypes =
    listOf(
      AudioDeviceInfo.TYPE_USB_DEVICE,
      AudioDeviceInfo.TYPE_USB_HEADSET,
      AudioDeviceInfo.TYPE_WIRED_HEADSET,
      AudioDeviceInfo.TYPE_BLUETOOTH_SCO,
    )

  /** The connected external mic to capture from when nothing is selected, if any. */
  @RequiresApi(Build.VERSION_CODES.M)
  private fun automaticInput(): AudioDeviceInfo? {
    val inputs = recordableInputs()

    return automaticInputTypes.firstNotNullOfOrNull { type -> inputs.firstOrNull { it.type == type } }
  }

  /**
   * The id of [automaticInput], or null to leave the choice to the platform.
   *
   * Unlike an explicit selection, a headset whose link does not come up is not an error:
   * the capture falls back to the platform's default input.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  private fun automaticInputId(): Int? {
    val input = automaticInput() ?: return null

    if (!prepareCaptureRouting(input)) {
      stopBluetoothSco()
      return null
    }

    return input.id
  }

  /** Gives back a route from [acquireInputRoute]; the last one tears the routing down. */
  fun releaseInputRoute() {
    val wasLastCapture =
      synchronized(inputSelectionLock) {
        acquiredInputRouteCount = maxOf(0, acquiredInputRouteCount - 1)
        acquiredInputRouteCount == 0
      }

    if (wasLastCapture) {
      stopBluetoothSco()
    }
  }

  @RequiresApi(Build.VERSION_CODES.M)
  private fun prepareCaptureRouting(input: AudioDeviceInfo): Boolean {
    if (input.type != AudioDeviceInfo.TYPE_BLUETOOTH_SCO) {
      return true
    }

    return startBluetoothSco(input) && awaitBluetoothScoConnected()
  }

  private fun awaitBluetoothScoConnected(): Boolean {
    val deadline = SystemClock.elapsedRealtime() + BLUETOOTH_SCO_CONNECT_TIMEOUT_MS

    while (!isBluetoothScoConnected()) {
      if (SystemClock.elapsedRealtime() >= deadline) {
        Log.w(TAG, "Bluetooth SCO link did not come up in time, the headset mic stays silent")
        return false
      }

      Thread.sleep(BLUETOOTH_SCO_POLL_INTERVAL_MS)
    }

    return true
  }

  private fun isBluetoothScoConnected(): Boolean {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      return audioManager.communicationDevice?.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO
    }

    // The state broadcast is sticky, so the last one can be read without a receiver.
    val lastScoState =
      reactContext.get()?.registerReceiver(null, IntentFilter(AudioManager.ACTION_SCO_AUDIO_STATE_UPDATED))

    return lastScoState?.getIntExtra(AudioManager.EXTRA_SCO_AUDIO_STATE, AudioManager.SCO_AUDIO_STATE_ERROR) ==
      AudioManager.SCO_AUDIO_STATE_CONNECTED
  }

  /**
   * Returns the selection to the system default when the selected device is among
   * [removedDevices]. A reconnected device gets a new id, so the old one would make
   * every later capture stream ask for a device that no longer exists.
   */
  fun forgetPreferredInputDeviceIfRemoved(removedDevices: Array<out AudioDeviceInfo>) {
    synchronized(inputSelectionLock) {
      val selectedInputDeviceId = preferredInputDeviceId ?: return

      if (removedDevices.none { it.id == selectedInputDeviceId }) {
        return
      }

      // Cleared even while a capture runs: the disconnect is tearing that stream down anyway.
      preferredInputDeviceId = null
    }

    stopBluetoothSco()
  }

  private fun hasPermission(permission: String): Boolean {
    val context = reactContext.get() ?: return false
    return ContextCompat.checkSelfPermission(context, permission) == PackageManager.PERMISSION_GRANTED
  }

  /**
   * A Bluetooth headset mic carries audio only over the SCO link, and the platform
   * brings that link up for the communication device, never for a capture stream's
   * preferred device. Without it the stream opens on the headset and receives no frames.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  private fun startBluetoothSco(input: AudioDeviceInfo): Boolean {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      val scoOutputs =
        audioManager.availableCommunicationDevices.filter {
          it.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO
        }
      val headset = scoOutputs.firstOrNull { it.address == input.address } ?: scoOutputs.firstOrNull()

      if (headset == null || !audioManager.setCommunicationDevice(headset)) {
        Log.w(TAG, "Cannot route communication to the Bluetooth headset, its mic stays silent")
        return false
      }
    } else {
      // Before Android 12 the request below is silently ignored without this permission,
      // and waiting for a link that never comes up would only delay the recording.
      if (!hasPermission(Manifest.permission.MODIFY_AUDIO_SETTINGS)) {
        Log.w(TAG, "MODIFY_AUDIO_SETTINGS is not granted, cannot use the Bluetooth headset mic")
        return false
      }

      @Suppress("DEPRECATION")
      audioManager.startBluetoothSco()
      @Suppress("DEPRECATION")
      audioManager.isBluetoothScoOn = true
    }

    startedBluetoothSco = true
    return true
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

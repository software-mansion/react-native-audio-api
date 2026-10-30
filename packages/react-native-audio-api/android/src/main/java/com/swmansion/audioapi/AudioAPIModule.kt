package com.swmansion.audioapi

import android.media.AudioManager
import android.os.Build
import android.util.Base64
import androidx.annotation.RequiresApi
import androidx.annotation.RequiresPermission
import com.facebook.jni.HybridData
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.LifecycleEventListener
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.common.annotations.FrameworkAPI
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl
import com.swmansion.audioapi.system.ForegroundServiceManager
import com.swmansion.audioapi.system.MediaSessionManager
import com.swmansion.audioapi.system.NativeFileInfo
import com.swmansion.audioapi.system.PermissionRequestListener
import java.lang.ref.WeakReference
import kotlin.concurrent.thread

@OptIn(FrameworkAPI::class)
@ReactModule(name = AudioAPIModule.NAME)
class AudioAPIModule(
  reactContext: ReactApplicationContext,
) : NativeAudioAPIModuleSpec(reactContext),
  LifecycleEventListener {
  companion object {
    const val NAME = NativeAudioAPIModuleSpec.NAME
    private const val TAG = "AudioAPIModule"
    private const val INPUT_DEVICE_ERROR = "E_INPUT_DEVICE"
  }

  val reactContext: WeakReference<ReactApplicationContext> = WeakReference(reactContext)

  private lateinit var mHybridData: HybridData

  @OptIn(markerClass = [FrameworkAPI::class])
  private external fun initHybrid(
    jsContext: Long,
    callInvoker: CallInvokerHolderImpl,
  ): HybridData

  @OptIn(markerClass = [FrameworkAPI::class])
  private external fun injectJSIBindings()

  external fun invokeHandlerWithEventNameAndEventBody(
    eventOrdinal: Int,
    eventBody: Map<String, Any>,
  )

  init {
    try {
      System.loadLibrary("react-native-audio-api")
    } catch (exception: UnsatisfiedLinkError) {
      throw RuntimeException("Could not load native module AudioAPIModule", exception)
    }
  }

  @OptIn(markerClass = [FrameworkAPI::class])
  override fun install(): Boolean {
    val context = reactContext.get() ?: return false
    context.assertOnJSQueueThread()

    val jsContext = context.javaScriptContextHolder!!.get()
    val jsCallInvokerHolder = context.jsCallInvokerHolder as CallInvokerHolderImpl

    mHybridData = initHybrid(jsContext, jsCallInvokerHolder)
    MediaSessionManager.initialize(WeakReference(this), reactContext)
    NativeFileInfo.initialize(reactContext)
    injectJSIBindings()

    return true
  }

  override fun onHostResume() {
    // do nothing
  }

  override fun onHostPause() {
    // do nothing
  }

  override fun onHostDestroy() {
    // do nothing
  }

  override fun initialize() {
    reactContext.get()?.addLifecycleEventListener(this)
  }

  override fun invalidate() {
    reactContext.get()?.removeLifecycleEventListener(this)
    ForegroundServiceManager.cleanup()
    MediaSessionManager.cleanup()
  }

  override fun getDevicePreferredSampleRate(): Double = MediaSessionManager.getDevicePreferredSampleRate()

  override fun getDeviceBufferDuration(): Double = MediaSessionManager.getDeviceBufferDuration()

  override fun isFfmpegEnabled(): Boolean = BuildConfig.RN_AUDIO_API_FFMPEG_ENABLED

  override fun setAudioSessionActivity(
    enabled: Boolean,
    promise: Promise?,
  ) {
    MediaSessionManager.setAudioSessionActivity(enabled) { error ->
      if (error == null) {
        promise?.resolve(null)
      } else {
        promise?.reject("E_COMMUNICATION_SESSION", error)
      }
    }
  }

  /** The iOS options are AVAudioSession settings and have no Android counterpart. */
  override fun setSystemOptions(
    ios: ReadableMap?,
    android: ReadableMap?,
  ) {
    MediaSessionManager.setSystemOptions(
      androidMode = android?.optionalString("mode"),
      androidCommunicationDevice = android?.optionalString("communicationDevice"),
    )
  }

  private fun ReadableMap.optionalString(key: String): String? = if (hasKey(key)) getString(key) else null

  override fun disableSessionManagement() {
    // nothing to do here
  }

  override fun observeAudioInterruptions(
    focusType: String?,
    enabled: Boolean,
  ) {
    val focus =
      when (focusType) {
        "gainTransient" -> AudioManager.AUDIOFOCUS_GAIN_TRANSIENT
        "gainTransientMayDuck" -> AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK
        "gainTransientExclusive" -> AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE
        else -> AudioManager.AUDIOFOCUS_GAIN
      }
    MediaSessionManager.observeAudioInterruptions(focus, enabled)
  }

  override fun activelyReclaimSession(enabled: Boolean) {
    MediaSessionManager.activelyReclaimSession(enabled)
  }

  override fun observeVolumeChanges(enabled: Boolean) {
    MediaSessionManager.observeVolumeChanges(enabled)
  }

  override fun getSystemVolume(): Double = MediaSessionManager.getSystemVolume()

  override fun requestRecordingPermissions(promise: Promise) {
    val permissionRequestListener = PermissionRequestListener(promise)
    MediaSessionManager.requestRecordingPermissions(permissionRequestListener)
  }

  override fun checkRecordingPermissions(promise: Promise) {
    promise.resolve(MediaSessionManager.checkRecordingPermissions())
  }

  override fun requestNotificationPermissions(promise: Promise) {
    val permissionRequestListener = PermissionRequestListener(promise)
    MediaSessionManager.requestNotificationPermissions(permissionRequestListener)
  }

  override fun checkNotificationPermissions(promise: Promise) {
    promise.resolve(MediaSessionManager.checkNotificationPermissions())
  }

  @RequiresApi(Build.VERSION_CODES.O)
  override fun getDevicesInfo(promise: Promise) {
    promise.resolve(MediaSessionManager.getDevicesInfo())
  }

  @RequiresApi(Build.VERSION_CODES.M)
  override fun setInputDevice(
    deviceId: String?,
    promise: Promise?,
  ) {
    val device = deviceId?.let { MediaSessionManager.inputRouter.findInputDevice(it) }

    if (deviceId != null && device == null) {
      promise?.reject(INPUT_DEVICE_ERROR, "Input device with id $deviceId not found", null)
      return
    }

    if (!MediaSessionManager.inputRouter.setPreferredInputDevice(device)) {
      promise?.reject(
        INPUT_DEVICE_ERROR,
        "The running recording could not continue on the selected input device.",
        null,
      )
      return
    }

    promise?.resolve(null)
  }

  override fun setCommunicationDevice(
    device: String?,
    promise: Promise?,
  ) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
      promise?.reject("E_UNSUPPORTED", "Communication-device routing requires Android 12 (API 31) or later")
      return
    }
    if (device == null) {
      promise?.reject("E_INVALID_COMMUNICATION_DEVICE", "A communication device is required")
      return
    }

    MediaSessionManager.setCommunicationDevice(device) { error ->
      if (error == null) {
        promise?.resolve(null)
      } else {
        promise?.reject("E_COMMUNICATION_DEVICE", error)
      }
    }
  }

  @RequiresApi(Build.VERSION_CODES.O)
  override fun getCommunicationDevice(promise: Promise?) {
    MediaSessionManager.getCommunicationDevice { device -> promise?.resolve(device) }
  }

  // Notification system methods
  @RequiresPermission(android.Manifest.permission.POST_NOTIFICATIONS)
  override fun showNotification(
    type: String?,
    key: String?,
    options: ReadableMap?,
    promise: Promise?,
  ) {
    val result = Arguments.createMap()
    try {
      if (type == null || key == null) {
        result.putBoolean("success", false)
        result.putString("error", "Type and key are required")
        promise?.resolve(result)
        return
      }

      MediaSessionManager.showNotification(type, key, options)

      result.putBoolean("success", true)
      promise?.resolve(result)
    } catch (e: Exception) {
      result.putBoolean("success", false)
      result.putString("error", e.message ?: "Unknown error")
      promise?.resolve(result)
    }
  }

  override fun hideNotification(
    key: String?,
    promise: Promise?,
  ) {
    try {
      if (key == null) {
        val result = Arguments.createMap()
        result.putBoolean("success", false)
        result.putString("error", "Key is required")
        promise?.resolve(result)
        return
      }

      MediaSessionManager.hideNotification(key)

      val result = Arguments.createMap()
      result.putBoolean("success", true)
      promise?.resolve(result)
    } catch (e: Exception) {
      val result = Arguments.createMap()
      result.putBoolean("success", false)
      result.putString("error", e.message ?: "Unknown error")
      promise?.resolve(result)
    }
  }

  override fun isNotificationActive(
    key: String?,
    promise: Promise?,
  ) {
    try {
      if (key == null) {
        promise?.resolve(false)
        return
      }

      val isActive = MediaSessionManager.isNotificationActive(key)
      promise?.resolve(isActive)
    } catch (_: Exception) {
      promise?.resolve(false)
    }
  }

  override fun readAndroidReleaseAssetBytesAsBase64(
    assetPath: String?,
    promise: Promise?,
  ) {
    if (assetPath.isNullOrBlank()) {
      promise?.reject("E_INVALID_ASSET", "Asset path is empty", null)
      return
    }
    val appContext = reactContext.get()?.applicationContext
    if (appContext == null) {
      promise?.reject("E_NO_CONTEXT", "React context unavailable", null)
      return
    }
    thread(name = "rnaa-read-release-asset") {
      try {
        val bytes = readAndroidBundledAssetBytes(appContext, assetPath)
        if (bytes == null || bytes.isEmpty()) {
          promise?.reject("E_READ_ASSET", "Could not read asset bytes", null)
          return@thread
        }
        promise?.resolve(Base64.encodeToString(bytes, Base64.NO_WRAP))
      } catch (e: Exception) {
        promise?.reject("E_READ_ASSET", e.message, e)
      }
    }
  }

  private fun readAndroidBundledAssetBytes(
    context: android.content.Context,
    assetPath: String,
  ): ByteArray? {
    val packageName = context.packageName

    var resId = context.resources.getIdentifier(assetPath, "raw", packageName)
    if (resId == 0 && assetPath.contains('.')) {
      val nameWithoutExt = assetPath.substringBeforeLast('.')
      resId = context.resources.getIdentifier(nameWithoutExt, "raw", packageName)
    }
    if (resId != 0) {
      context.resources.openRawResource(resId).use { return it.readBytes() }
    }
    return null
  }
}

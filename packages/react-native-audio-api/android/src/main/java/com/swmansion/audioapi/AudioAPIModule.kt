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
import com.facebook.react.bridge.ReadableArray
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
  }

  val reactContext: WeakReference<ReactApplicationContext> = WeakReference(reactContext)

  private lateinit var mHybridData: HybridData

  @OptIn(markerClass = [FrameworkAPI::class])
  private external fun initHybrid(
    workletsModule: Any?,
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

    var workletsModule: Any? = null
    if (BuildConfig.RN_AUDIO_API_ENABLE_WORKLETS) {
      try {
        workletsModule = context.getNativeModule("WorkletsModule")
      } catch (_: Exception) {
        throw RuntimeException("WorkletsModule not found - make sure react-native-worklets is properly installed")
      }
    }

    mHybridData = initHybrid(workletsModule, jsContext, jsCallInvokerHolder)
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
    // Cleanup foreground service manager
    ForegroundServiceManager.cleanup()
  }

  override fun getDevicePreferredSampleRate(): Double = MediaSessionManager.getDevicePreferredSampleRate()

  override fun setAudioSessionActivity(
    enabled: Boolean,
    promise: Promise?,
  ) {
    promise?.resolve(true)
  }

  override fun setAudioSessionOptions(
    category: String?,
    mode: String?,
    options: ReadableArray?,
    allowHaptics: Boolean,
    notifyOthersOnDeactivation: Boolean,
  ) {
    // noting to do here
  }

  override fun disableSessionManagement() {
    // nothing to do here
  }

  override fun observeAudioInterruptions(
    focusType: String?,
    enabled: Boolean,
  ) {
    if (!enabled) {
      MediaSessionManager.abandonAudioFocus()
      return
    }
    when (focusType) {
      "gain" -> MediaSessionManager.requestAudioFocus(AudioManager.AUDIOFOCUS_GAIN)
      "gainTransient" -> MediaSessionManager.requestAudioFocus(AudioManager.AUDIOFOCUS_GAIN_TRANSIENT)
      "gainTransientMayDuck" -> MediaSessionManager.requestAudioFocus(AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)
      "gainTransientExclusive" -> MediaSessionManager.requestAudioFocus(AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE)
      else -> MediaSessionManager.requestAudioFocus(AudioManager.AUDIOFOCUS_GAIN)
    }
  }

  override fun activelyReclaimSession(enabled: Boolean) {
    MediaSessionManager.activelyReclaimSession(enabled)
  }

  override fun observeVolumeChanges(enabled: Boolean) {
    MediaSessionManager.observeVolumeChanges(enabled)
  }

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

  override fun setInputDevice(
    deviceId: String?,
    promise: Promise?,
  ) {
    // TODO: noop for now, but it should be moved to upcoming
    // audio engine implementation for android (duplex stream)
    promise?.resolve(true)
  }

  // Notification system methods
  @RequiresPermission(android.Manifest.permission.POST_NOTIFICATIONS)
  override fun showNotification(
    type: String?,
    key: String?,
    options: ReadableMap?,
    promise: Promise?,
  ) {
    try {
      if (type == null || key == null) {
        val result = Arguments.createMap()
        result.putBoolean("success", false)
        result.putString("error", "Type and key are required")
        promise?.resolve(result)
        return
      }

      MediaSessionManager.showNotification(type, key, options)

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

package com.swmansion.audioapi

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

@OptIn(FrameworkAPI::class)
@ReactModule(name = AudioAPIModule.NAME)
class AudioAPIModule(
  reactContext: ReactApplicationContext,
) : NativeAudioAPIModuleSpec(reactContext),
  LifecycleEventListener {
  companion object {
    const val NAME = NativeAudioAPIModuleSpec.NAME
  }

  val reactContext: WeakReference<ReactApplicationContext> = WeakReference(reactContext)

  private val mHybridData: HybridData

  external fun initHybrid(
    workletsModule: Any?,
    jsContext: Long,
    callInvoker: CallInvokerHolderImpl,
  ): HybridData

  private external fun injectJSIBindings()

  external fun invokeHandlerWithEventNameAndEventBody(
    eventName: String,
    eventBody: Map<String, Any>,
  )

  init {
    try {
      System.loadLibrary("react-native-audio-api")
      val jsCallInvokerHolder = reactContext.jsCallInvokerHolder as CallInvokerHolderImpl

      var workletsModule: Any? = null
      if (BuildConfig.RN_AUDIO_API_ENABLE_WORKLETS) {
        try {
          workletsModule = reactContext.getNativeModule("WorkletsModule")
        } catch (ex: Exception) {
          throw RuntimeException("WorkletsModule not found - make sure react-native-worklets is properly installed")
        }
      }
      mHybridData = initHybrid(workletsModule, reactContext.javaScriptContextHolder!!.get(), jsCallInvokerHolder)
    } catch (exception: UnsatisfiedLinkError) {
      throw RuntimeException("Could not load native module AudioAPIModule", exception)
    }
  }

  override fun install(): Boolean {
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
  ) {
    // noting to do here
  }

  override fun disableSessionManagement() {
    // nothing to do here
  }

  override fun observeAudioInterruptions(enabled: Boolean) {
    MediaSessionManager.observeAudioInterruptions(enabled)
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
    } catch (e: Exception) {
      promise?.resolve(false)
    }
  }
}

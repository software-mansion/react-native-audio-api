package com.swmansion.audioapi

import com.facebook.jni.HybridData
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableArray
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.common.annotations.FrameworkAPI
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl
import com.swmansion.audioapi.system.MediaSessionManager

@OptIn(FrameworkAPI::class)
@ReactModule(name = AudioAPIModule.NAME)
class AudioAPIModule(
  val reactContext: ReactApplicationContext,
) : NativeAudioAPIModuleSpec(reactContext) {
  companion object {
    const val NAME = NativeAudioAPIModuleSpec.NAME
  }

  private val mHybridData: HybridData

  external fun initHybrid(
    jsContext: Long,
    callInvoker: CallInvokerHolderImpl,
  ): HybridData

  private external fun injectJSIBindings()

  init {
    try {
      System.loadLibrary("react-native-audio-api")
      val jsCallInvokerHolder = reactContext.jsCallInvokerHolder as CallInvokerHolderImpl
      mHybridData = initHybrid(reactContext.javaScriptContextHolder!!.get(), jsCallInvokerHolder)
    } catch (exception: UnsatisfiedLinkError) {
      throw RuntimeException("Could not load native module AudioAPIModule", exception)
    }
  }

  override fun install(): Boolean {
    MediaSessionManager.initialize(reactContext)
    injectJSIBindings()

    return true
  }

  override fun setLockScreenInfo(info: ReadableMap?) {
    MediaSessionManager.setLockScreenInfo(info)
  }

  override fun resetLockScreenInfo() {
    MediaSessionManager.resetLockScreenInfo()
  }

  override fun enableRemoteCommand(
    name: String?,
    enabled: Boolean,
  ) {
    MediaSessionManager.enableRemoteCommand(name!!, enabled)
  }

  override fun setAudioSessionOptions(
    category: String?,
    mode: String?,
    options: ReadableArray?,
  ) {
    // noting to do here
  }

  override fun getDevicePreferredSampleRate(): Double = MediaSessionManager.getDevicePreferredSampleRate()

  override fun observeAudioInterruptions(enabled: Boolean) {
    MediaSessionManager.observeAudioInterruptions(enabled)
  }

  override fun observeVolumeChanges(enabled: Boolean) {
    MediaSessionManager.observeVolumeChanges(enabled)
  }

  override fun requestRecordingPermissions(promise: Promise?) {
    val res = MediaSessionManager.requestRecordingPermissions(currentActivity)
    promise!!.resolve(res)
  }

  override fun checkRecordingPermissions(promise: Promise?) {
    val res = MediaSessionManager.checkRecordingPermissions()
    promise!!.resolve(res)
  }
}

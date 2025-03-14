package com.swmansion.audioapi

import com.facebook.jni.HybridData
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.common.annotations.FrameworkAPI
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl

@OptIn(FrameworkAPI::class)
@ReactModule(name = AudioAPIModule.NAME)
class AudioAPIModule(
  reactContext: ReactApplicationContext,
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

  @ReactMethod(isBlockingSynchronousMethod = true)
  override fun install(): Boolean {
    injectJSIBindings()

    return true
  }
}

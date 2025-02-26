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
    init {
      System.loadLibrary("react-native-audio-api")
    }

    const val NAME = NativeAudioAPIModuleSpec.NAME
  }

  private val mHybridData: HybridData

  external fun initHybrid(
    jsContext: Long,
    callInvoker: CallInvokerHolderImpl,
  ): HybridData

  private external fun injectJSIBindings()

  init {
    val jsCallInvokerHolder = reactContext.jsCallInvokerHolder as CallInvokerHolderImpl
    mHybridData = initHybrid(reactContext.javaScriptContextHolder!!.get(), jsCallInvokerHolder)
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  override fun install(): Boolean {
    injectJSIBindings()

    return true
  }
}

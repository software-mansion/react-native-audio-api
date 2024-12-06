package com.swmansion.audioapi.module

import com.facebook.jni.HybridData
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.common.annotations.FrameworkAPI
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl

@OptIn(FrameworkAPI::class)
class AudioAPIInstaller(
  reactContext: ReactApplicationContext,
) {
  private val mHybridData: HybridData?

  companion object {
    init {
      System.loadLibrary("react-native-audio-api")
    }
  }

  init {
    val jsCallInvokerHolder = reactContext.jsCallInvokerHolder as CallInvokerHolderImpl
    mHybridData = initHybrid(reactContext.javaScriptContextHolder!!.get(), jsCallInvokerHolder)
  }

  external fun initHybrid(
    jsContext: Long,
    callInvoker: CallInvokerHolderImpl,
  ): HybridData?

  external fun install()
}

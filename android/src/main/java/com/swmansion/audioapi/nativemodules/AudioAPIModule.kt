package com.swmansion.audioapi.nativemodules

import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.swmansion.audioapi.module.AudioAPI

class AudioAPIModule(
  private val reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  override fun getName(): String = NAME

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun install(): Boolean {
    val audioAPI = AudioAPI()
    val jsContext = reactContext.javaScriptContextHolder!!.get()
    audioAPI.install(jsContext)

    return true
  }

  companion object {
    const val NAME: String = "AudioAPIModule"
  }
}

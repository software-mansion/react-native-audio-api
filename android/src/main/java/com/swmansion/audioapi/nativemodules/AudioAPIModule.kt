package com.swmansion.audioapi.nativemodules

import com.swmansion.audioapi.context.AudioContext
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod

class AudioAPIModule(
  private val reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  override fun getName(): String = NAME

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun install(): Boolean {
    val audioContext = AudioContext()
    val jsContext = reactContext.javaScriptContextHolder!!.get()
    audioContext.install(jsContext)

    return true
  }

  companion object {
    const val NAME: String = "AudioAPIModule"
  }
}

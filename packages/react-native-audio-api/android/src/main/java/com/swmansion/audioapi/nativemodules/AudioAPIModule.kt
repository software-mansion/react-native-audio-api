package com.swmansion.audioapi.nativemodules

import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.swmansion.audioapi.module.AudioAPIInstaller

class AudioAPIModule(
  private val reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  override fun getName(): String = NAME

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun install(): Boolean {
    val audioAPIInstaller = AudioAPIInstaller()
    val jsContext = reactContext.javaScriptContextHolder!!.get()
    audioAPIInstaller.install(jsContext)

    return true
  }

  companion object {
    const val NAME: String = "AudioAPIModule"
  }
}

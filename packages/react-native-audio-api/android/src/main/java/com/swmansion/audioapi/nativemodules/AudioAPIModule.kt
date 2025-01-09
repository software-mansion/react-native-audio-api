package com.swmansion.audioapi.nativemodules

import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.module.annotations.ReactModule
import com.swmansion.audioapi.module.AudioAPIInstaller

@ReactModule(name = AudioAPIModule.NAME)
class AudioAPIModule(
  private val reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  companion object {
    const val NAME: String = "AudioAPIModule"
  }

  override fun getName(): String = NAME

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun install(): Boolean {
    val audioAPIInstaller = AudioAPIInstaller(reactContext)
    audioAPIInstaller.install()

    return true
  }
}

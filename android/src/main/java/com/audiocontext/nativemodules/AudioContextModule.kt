package com.audiocontext.nativemodules

import android.util.Log
import com.audiocontext.context.AudioContext
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod

class AudioContextModule(private val reactContext: ReactApplicationContext) : ReactContextBaseJavaModule(reactContext) {

  override fun getName(): String {
    return NAME
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun install(): Boolean {
    val audioContext = AudioContext()
    val jsContext = reactContext.javaScriptContextHolder!!.get()
    audioContext.install(jsContext)

    return true
  }

  companion object {
    const val NAME: String = "AudioContextModule"
  }
}

package com.swmansion.audioapi.system

import com.facebook.react.bridge.ReactApplicationContext
import java.lang.ref.WeakReference

object NativeFileInfo {
  private lateinit var reactContext: WeakReference<ReactApplicationContext>

  fun initialize(reactContext: WeakReference<ReactApplicationContext>) {
    this.reactContext = reactContext
  }

  @JvmStatic
  fun getFilesDir(): String = reactContext.get()?.filesDir?.absolutePath ?: ""

  @JvmStatic
  fun getCacheDir(): String = reactContext.get()?.cacheDir?.absolutePath ?: ""
}

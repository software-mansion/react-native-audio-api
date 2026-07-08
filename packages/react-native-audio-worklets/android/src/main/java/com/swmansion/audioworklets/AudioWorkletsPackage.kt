package com.swmansion.audioworklets

import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfoProvider

/**
 * Stub [BaseReactPackage] so React Native autolinking can include this library as a Gradle
 * project. The TurboModule is a pure C++ module registered via autolinking's
 * `autolinking_cxxModuleProvider` (see `react-native.config.js` cxxModule* fields).
 */
class AudioWorkletsPackage : BaseReactPackage() {
  override fun getModule(
    name: String,
    reactContext: ReactApplicationContext,
  ): NativeModule? = null

  override fun getReactModuleInfoProvider(): ReactModuleInfoProvider = ReactModuleInfoProvider { emptyMap() }
}

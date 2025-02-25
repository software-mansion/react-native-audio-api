package com.swmansion.audioapi

import com.facebook.react.BaseReactPackage
import com.facebook.react.ReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.common.annotations.UnstableReactNativeAPI
import com.facebook.react.module.annotations.ReactModuleList
import com.facebook.react.module.model.ReactModuleInfo
import com.facebook.react.module.model.ReactModuleInfoProvider

@ReactModuleList(nativeModules = [AudioAPIModule::class])
class AudioAPIPackage :
  BaseReactPackage(),
  ReactPackage {
  @UnstableReactNativeAPI
  override fun getModule(
    name: String,
    reactContext: ReactApplicationContext,
  ): NativeModule? =
    when (name) {
      AudioAPIModule.NAME -> AudioAPIModule(reactContext)
      else -> null
    }

  override fun getReactModuleInfoProvider(): ReactModuleInfoProvider =
    ReactModuleInfoProvider(
      fun (): Map<String, ReactModuleInfo> =
        mapOf(
          AudioAPIModule.NAME to
            ReactModuleInfo(
              AudioAPIModule.NAME,
              AudioAPIModule::class.java.simpleName,
              _canOverrideExistingModule = true,
              _needsEagerInit = false,
              isCxxModule = true,
              isTurboModule = BuildConfig.IS_NEW_ARCHITECTURE_ENABLED,
            ),
        ),
    )
}

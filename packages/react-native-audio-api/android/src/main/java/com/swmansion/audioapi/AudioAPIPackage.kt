package com.swmansion.audioapi

import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.annotations.ReactModuleList
import com.facebook.react.module.model.ReactModuleInfo
import com.facebook.react.module.model.ReactModuleInfoProvider

@ReactModuleList(
  nativeModules = [
    AudioAPIModule::class,
    AudioManagerModule::class,
  ],
)
class AudioAPIPackage : BaseReactPackage() {
  override fun getModule(
    name: String,
    reactContext: ReactApplicationContext,
  ): NativeModule? {
    when (name) {
      AudioAPIModule.NAME -> return AudioAPIModule(reactContext)
      AudioManagerModule.NAME -> return AudioManagerModule(reactContext)
    }
    return null
  }

  override fun getReactModuleInfoProvider(): ReactModuleInfoProvider =
    ReactModuleInfoProvider {
      val moduleInfos: MutableMap<String, ReactModuleInfo> = HashMap()
      val isTurboModule = BuildConfig.IS_NEW_ARCHITECTURE_ENABLED
      moduleInfos[AudioAPIModule.NAME] =
        ReactModuleInfo(
          AudioAPIModule.NAME,
          AudioAPIModule.NAME,
          canOverrideExistingModule = true,
          needsEagerInit = false,
          isCxxModule = false,
          isTurboModule = isTurboModule,
        )

      moduleInfos[AudioManagerModule.NAME] =
        ReactModuleInfo(
          AudioManagerModule.NAME,
          AudioManagerModule.NAME,
          canOverrideExistingModule = true,
          needsEagerInit = false,
          isCxxModule = false,
          isTurboModule = isTurboModule,
        )
      moduleInfos
    }
}

package com.swmansion.audioapi;

import androidx.annotation.NonNull;
import com.facebook.react.BaseReactPackage;
import com.facebook.react.ReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.annotations.ReactModuleList;
import com.facebook.react.module.model.ReactModuleInfo;
import com.facebook.react.module.model.ReactModuleInfoProvider;
import java.util.Map;

@ReactModuleList(nativeModules = {AudioAPIModule.class})
public class AudioAPIPackage extends BaseReactPackage implements ReactPackage {
  @Override
  public NativeModule getModule(
    @NonNull String name, @NonNull ReactApplicationContext reactContext) {
    return switch (name) {
      case AudioAPIModule.NAME -> new AudioAPIModule(reactContext);
      default -> null;
    };
  }

  @Override
  public ReactModuleInfoProvider getReactModuleInfoProvider() {
    return new ReactModuleInfoProvider() {
      @NonNull
      @Override
      public Map<String, ReactModuleInfo> getReactModuleInfos() {
        return Map.of(AudioAPIModule.NAME, new ReactModuleInfo(
          AudioAPIModule.NAME,
          AudioAPIModule.class.getName(),
          true,
          false,
          true,
          BuildConfig.IS_NEW_ARCHITECTURE_ENABLED
        ));
      }
    };
  }
}

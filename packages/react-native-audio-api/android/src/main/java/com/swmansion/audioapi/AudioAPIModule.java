package com.swmansion.audioapi;

import androidx.annotation.OptIn;
import com.facebook.jni.HybridData;
import com.facebook.proguard.annotations.DoNotStrip;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.common.annotations.FrameworkAPI;
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl;
import com.facebook.soloader.SoLoader;
import java.util.Objects;

public class AudioAPIModule extends NativeAudioAPIModuleSpec {
  static {
    SoLoader.loadLibrary("react-native-audio-api");
  }

  @DoNotStrip
  @SuppressWarnings("unused")
  private HybridData mHybridData;

  @SuppressWarnings("unused")
  protected HybridData getHybridData() {
    return mHybridData;
  }

  @OptIn(markerClass = FrameworkAPI.class)
  private native HybridData initHybrid(
    long jsContext,
    CallInvokerHolderImpl jsCallInvokerHolder);

  @OptIn(markerClass = FrameworkAPI.class)
  private native void injectJSIBindings();

  public AudioAPIModule(ReactApplicationContext reactContext) {
    super(reactContext);
  }

  @OptIn(markerClass = FrameworkAPI.class)
  @ReactMethod(isBlockingSynchronousMethod = true)
  public boolean install() {
    var reactContext = getReactApplicationContext();
    var jsContext = Objects.requireNonNull(reactContext.getJavaScriptContextHolder()).get();
    var jsCallInvokerHolder = (CallInvokerHolderImpl) reactContext.getJSCallInvokerHolder();

    mHybridData = initHybrid(jsContext, jsCallInvokerHolder);
    injectJSIBindings();

    return true;
  }
}

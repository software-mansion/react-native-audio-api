package com.swmansion.audioapi.core

import com.facebook.common.internal.DoNotStrip
import com.swmansion.audioapi.system.ForegroundServiceManager
import java.util.UUID

@DoNotStrip
class NativeAudioRecorder {
  private var recorderId: String? = null

  @DoNotStrip
  fun start() {
    if (recorderId == null) {
      recorderId = UUID.randomUUID().toString()
      ForegroundServiceManager.subscribe("recorder_$recorderId")
    }
  }

  @DoNotStrip
  fun stop() {
    recorderId?.let {
      ForegroundServiceManager.unsubscribe("recorder_$it")
      recorderId = null
    }
  }
}

package com.swmansion.audioapi.core

import com.facebook.common.internal.DoNotStrip
import com.swmansion.audioapi.system.MediaSessionManager

@DoNotStrip
class NativeAudioRecorder {
  private var inputNodeId: String? = null

  @DoNotStrip
  fun start() {
    this.inputNodeId = MediaSessionManager.attachAudioRecorder(this)
    MediaSessionManager.startForegroundServiceIfNecessary()
  }

  @DoNotStrip
  fun stop() {
    this.inputNodeId?.let {
      MediaSessionManager.detachAudioRecorder(it)
      this.inputNodeId = null
    }
    MediaSessionManager.stopForegroundServiceIfNecessary()
  }
}

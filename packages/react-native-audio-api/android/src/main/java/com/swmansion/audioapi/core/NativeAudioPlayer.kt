package com.swmansion.audioapi.core

import com.facebook.common.internal.DoNotStrip
import com.swmansion.audioapi.system.MediaSessionManager

@DoNotStrip
class NativeAudioPlayer {
  private var sourceNodeId: String? = null

  @DoNotStrip
  fun start() {
    this.sourceNodeId = MediaSessionManager.attachAudioPlayer(this)
    MediaSessionManager.startForegroundServiceIfNecessary()
  }

  @DoNotStrip
  fun stop() {
    this.sourceNodeId?.let {
      MediaSessionManager.detachAudioPlayer(it)
      this.sourceNodeId = null
    }
    MediaSessionManager.stopForegroundServiceIfNecessary()
  }
}

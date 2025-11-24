package com.swmansion.audioapi.core

import com.facebook.common.internal.DoNotStrip
import com.swmansion.audioapi.system.ForegroundServiceManager
import java.util.UUID

@DoNotStrip
class NativeAudioPlayer {
  private var playerId: String? = null

  @DoNotStrip
  fun start() {
    if (playerId == null) {
      playerId = UUID.randomUUID().toString()
      ForegroundServiceManager.subscribe("player_$playerId")
    }
  }

  @DoNotStrip
  fun stop() {
    playerId?.let {
      ForegroundServiceManager.unsubscribe("player_$it")
      playerId = null
    }
  }
}

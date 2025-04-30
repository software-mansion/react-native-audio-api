package com.swmansion.audioapi.system

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.media.AudioManager

class VolumeChangeListener(
  private val audioManager: AudioManager,
  private val eventEmitter: MediaSessionEventEmitter,
) : BroadcastReceiver() {
  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    val currentVolume = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC).toDouble()
    val maxVolume = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC).toDouble()
    eventEmitter.onVolumeChange(mapOf("value" to currentVolume / maxVolume))
  }

  fun getIntentFilter(): IntentFilter {
    val intentFilter = IntentFilter()
    intentFilter.addAction("android.media.VOLUME_CHANGED_ACTION")
    return intentFilter
  }
}

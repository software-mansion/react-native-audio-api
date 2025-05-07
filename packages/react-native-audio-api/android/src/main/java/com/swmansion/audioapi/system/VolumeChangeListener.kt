package com.swmansion.audioapi.system

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.media.AudioManager
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference

class VolumeChangeListener(
  private val audioManager: AudioManager,
  audioAPIModule: AudioAPIModule,
) : BroadcastReceiver() {
  private val audioAPIModule: WeakReference<AudioAPIModule> = WeakReference(audioAPIModule)

  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    val currentVolume = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC).toDouble()
    val maxVolume = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC).toDouble()

//    val body = mapOf("value" to currentVolume / maxVolume)
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("volumeChange", mapOf())
  }

  fun getIntentFilter(): IntentFilter {
    val intentFilter = IntentFilter()
    intentFilter.addAction("android.media.VOLUME_CHANGED_ACTION")
    return intentFilter
  }
}

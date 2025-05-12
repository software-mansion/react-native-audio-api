package com.swmansion.audioapi.system

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.media.AudioManager
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference
import java.util.HashMap

class VolumeChangeListener(
  private val audioManager: WeakReference<AudioManager>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
) : BroadcastReceiver() {
  override fun onReceive(
    context: Context?,
    intent: Intent?,
  ) {
    val currentVolume = audioManager.get()?.getStreamVolume(AudioManager.STREAM_MUSIC)?.toDouble()!!
    val maxVolume = audioManager.get()?.getStreamMaxVolume(AudioManager.STREAM_MUSIC)?.toDouble()!!

    val body = HashMap<String, Any>().apply { put("value", currentVolume / maxVolume) }
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("volumeChange", body)
  }

  fun getIntentFilter(): IntentFilter {
    val intentFilter = IntentFilter()
    intentFilter.addAction("android.media.VOLUME_CHANGED_ACTION")
    return intentFilter
  }
}

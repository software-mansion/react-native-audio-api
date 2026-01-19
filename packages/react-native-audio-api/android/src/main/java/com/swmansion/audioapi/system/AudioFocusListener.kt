package com.swmansion.audioapi.system

import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build
import android.util.Log
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference
import java.util.HashMap

class AudioFocusListener(
  private val audioManager: WeakReference<AudioManager>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
) : AudioManager.OnAudioFocusChangeListener {
  private var focusRequest: AudioFocusRequest? = null
  private var isTransientLoss: Boolean = false

  override fun onAudioFocusChange(focusChange: Int) {
    Log.d("AudioFocusListener", "onAudioFocusChange: $focusChange")
    when (focusChange) {
      AudioManager.AUDIOFOCUS_LOSS -> {
        val body =
          HashMap<String, Any>().apply {
            put("type", "began")
            put("shouldResume", false)
            isTransientLoss = false
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
      }

      AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
        val body =
          HashMap<String, Any>().apply {
            put("type", "began")
            put("shouldResume", false)
            isTransientLoss = true
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
      }

      AudioManager.AUDIOFOCUS_GAIN -> {
        val body =
          HashMap<String, Any>().apply {
            put("type", "ended")
            put("shouldResume", isTransientLoss)
            isTransientLoss = false
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
      }

      AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("duck", emptyMap())
      }
    }
  }

  fun requestAudioFocus(focus: Int) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      this.focusRequest =
        AudioFocusRequest
          .Builder(focus)
          .setOnAudioFocusChangeListener(this)
          .build()

      audioManager.get()?.requestAudioFocus(focusRequest!!)
    } else {
      audioManager.get()?.requestAudioFocus(this, AudioManager.STREAM_MUSIC, focus)
    }
  }

  fun abandonAudioFocus() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && this.focusRequest != null) {
      audioManager.get()?.abandonAudioFocusRequest(focusRequest!!)
    } else {
      audioManager.get()?.abandonAudioFocus(this)
    }
  }
}

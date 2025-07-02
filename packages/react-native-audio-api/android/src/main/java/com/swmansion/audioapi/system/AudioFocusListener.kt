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
  private val lockScreenManager: WeakReference<LockScreenManager>,
) : AudioManager.OnAudioFocusChangeListener {
  private var playOnAudioFocus: Boolean = false
  private var focusRequest: AudioFocusRequest? = null

  override fun onAudioFocusChange(focusChange: Int) {
    Log.d("AudioFocusListener", "onAudioFocusChange: $focusChange")
    when (focusChange) {
      AudioManager.AUDIOFOCUS_LOSS -> {
        playOnAudioFocus = false
        val body =
          HashMap<String, Any>().apply {
            put("type", "began")
            put("shouldResume", false)
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
      }
      AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
        playOnAudioFocus = lockScreenManager.get()?.isPlaying == true
        val body =
          HashMap<String, Any>().apply {
            put("type", "began")
            put("shouldResume", playOnAudioFocus)
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
      }
      AudioManager.AUDIOFOCUS_GAIN -> {
        if (playOnAudioFocus) {
          val body =
            HashMap<String, Any>().apply {
              put("type", "ended")
              put("shouldResume", true)
            }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
        } else {
          val body =
            HashMap<String, Any>().apply {
              put("type", "ended")
              put("shouldResume", false)
            }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
        }

        playOnAudioFocus = false
      }
    }
  }

  fun requestAudioFocus() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      this.focusRequest =
        AudioFocusRequest
          .Builder(AudioManager.AUDIOFOCUS_GAIN)
          .setOnAudioFocusChangeListener(this)
          .build()

      audioManager.get()?.requestAudioFocus(focusRequest!!)
    } else {
      audioManager.get()?.requestAudioFocus(this, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN)
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

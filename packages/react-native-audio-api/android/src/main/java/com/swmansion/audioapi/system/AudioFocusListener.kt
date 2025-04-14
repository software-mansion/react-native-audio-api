package com.swmansion.audioapi.system

import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build
import android.util.Log

class AudioFocusListener(
  private val audioManager: AudioManager,
  val eventEmitter: MediaSessionEventEmitter,
  private val lockScreenManager: LockScreenManager,
) : AudioManager.OnAudioFocusChangeListener {
  private var playOnAudioFocus = false
  private var focusRequest: AudioFocusRequest? = null

  override fun onAudioFocusChange(focusChange: Int) {
    Log.d("AudioFocusListener", "onAudioFocusChange: $focusChange")
    when (focusChange) {
      AudioManager.AUDIOFOCUS_LOSS -> {
        playOnAudioFocus = false
        eventEmitter.onInterruption(mapOf("type" to "began", "shouldResume" to false))
      }
      AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
        playOnAudioFocus = lockScreenManager.isPlaying
        eventEmitter.onInterruption(mapOf("type" to "began", "shouldResume" to playOnAudioFocus))
      }
      AudioManager.AUDIOFOCUS_GAIN -> {
        if (playOnAudioFocus) {
          eventEmitter.onInterruption(mapOf("type" to "ended", "shouldResume" to true))
        } else {
          eventEmitter.onInterruption(mapOf("type" to "ended", "shouldResume" to false))
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

      audioManager.requestAudioFocus(focusRequest!!)
    } else {
      audioManager.requestAudioFocus(this, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN)
    }
  }

  fun abandonAudioFocus() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && this.focusRequest != null) {
      audioManager.abandonAudioFocusRequest(focusRequest!!)
    } else {
      audioManager.abandonAudioFocus(this)
    }
  }
}

package com.swmansion.audioapi.system

import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build
import android.util.Log


class AudioFocusListener(private val audioManager: AudioManager, val eventEmitter: MediaSessionEventEmitter, private val lockScreenManager: LockScreenManager) : AudioManager.OnAudioFocusChangeListener {
  private var playOnAudioFocus = false
  private var focusRequest: AudioFocusRequest? = null

  override fun onAudioFocusChange(focusChange: Int) {
    Log.d("AudioFocusListener", "onAudioFocusChange: $focusChange")
    if (focusChange == AudioManager.AUDIOFOCUS_LOSS) {
      abandonAudioFocus()
      playOnAudioFocus = false
      eventEmitter.onStop()
    } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
      if (lockScreenManager.isPlaying) {
        playOnAudioFocus = true
        eventEmitter.onPause()
      }
    } else if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
      if (playOnAudioFocus) {
        eventEmitter.onPlay()
      }
      playOnAudioFocus = false
    }
  }

  fun requestAudioFocus() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      this.focusRequest = AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
        .setOnAudioFocusChangeListener(this).build()

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

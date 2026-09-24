package com.swmansion.audioapi.system

import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build
import android.os.Handler
import android.util.Log
import androidx.annotation.RequiresApi
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference
import java.util.HashMap

class AudioFocusListener(
  private val audioManager: WeakReference<AudioManager>,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
  private val mainHandler: Handler,
  private val isCommunicationSessionActive: () -> Boolean,
) : AudioManager.OnAudioFocusChangeListener {
  private var focusRequest: AudioFocusRequest? = null
  private var hasLegacyFocusRequest = false
  private var isTransientLoss: Boolean = false
  private var communicationFocusRequest: AudioFocusRequest? = null

  override fun onAudioFocusChange(focusChange: Int) {
    val hasCommunicationFocus = communicationFocusRequest != null
    if (focusRequest == null && !hasLegacyFocusRequest && !hasCommunicationFocus) {
      return
    }
    if (hasCommunicationFocus && !isCommunicationSessionActive()) {
      return
    }

    Log.d("AudioFocusListener", "onAudioFocusChange: $focusChange")
    when (focusChange) {
      AudioManager.AUDIOFOCUS_LOSS -> {
        val body =
          HashMap<String, Any>().apply {
            put("type", "began")
            put("shouldResume", false)
            isTransientLoss = false
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.INTERRUPTION.ordinal, body)
      }

      AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
        val body =
          HashMap<String, Any>().apply {
            put("type", "began")
            put("shouldResume", false)
            isTransientLoss = true
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.INTERRUPTION.ordinal, body)
      }

      AudioManager.AUDIOFOCUS_GAIN -> {
        val body =
          HashMap<String, Any>().apply {
            put("type", "ended")
            put("shouldResume", isTransientLoss)
            isTransientLoss = false
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.INTERRUPTION.ordinal, body)
      }

      AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
        isTransientLoss = communicationFocusRequest != null
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.DUCK.ordinal, emptyMap())
      }
    }
  }

  fun requestAudioFocus(focus: Int) {
    if (communicationFocusRequest != null) {
      return
    }
    abandonAudioFocus()
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      hasLegacyFocusRequest = false
      this.focusRequest =
        AudioFocusRequest
          .Builder(focus)
          .setOnAudioFocusChangeListener(this, mainHandler)
          .build()

      audioManager.get()?.requestAudioFocus(focusRequest!!)
    } else {
      val result = audioManager.get()?.requestAudioFocus(this, AudioManager.STREAM_MUSIC, focus)
      hasLegacyFocusRequest = result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED
    }
  }

  fun abandonAudioFocus() {
    if (communicationFocusRequest != null) {
      return
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && this.focusRequest != null) {
      audioManager.get()?.abandonAudioFocusRequest(focusRequest!!)
      focusRequest = null
    } else {
      audioManager.get()?.abandonAudioFocus(this)
      hasLegacyFocusRequest = false
    }
    isTransientLoss = false
  }

  @RequiresApi(Build.VERSION_CODES.O)
  fun requestCommunicationAudioFocus(): Boolean {
    if (communicationFocusRequest != null) {
      return true
    }

    abandonAudioFocus()

    val request =
      AudioFocusRequest
        .Builder(AudioManager.AUDIOFOCUS_GAIN_TRANSIENT)
        .setAudioAttributes(
          AudioAttributes
            .Builder()
            .setUsage(AudioAttributes.USAGE_VOICE_COMMUNICATION)
            .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
            .build(),
        ).setAcceptsDelayedFocusGain(false)
        .setWillPauseWhenDucked(true)
        .setOnAudioFocusChangeListener(this, mainHandler)
        .build()

    val result = audioManager.get()?.requestAudioFocus(request)
    if (result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
      communicationFocusRequest = request
      return true
    }
    return false
  }

  @RequiresApi(Build.VERSION_CODES.O)
  fun abandonCommunicationAudioFocus() {
    communicationFocusRequest?.let { audioManager.get()?.abandonAudioFocusRequest(it) }
    communicationFocusRequest = null
    isTransientLoss = false
  }
}

package com.swmansion.audioapi.system

import android.media.AudioAttributes
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
  private var playOnAudioFocus = false
  private var focusRequest: AudioFocusRequest? = null

  override fun onAudioFocusChange(focusChange: Int) {
    Log.d("AudioFocusListener", "onAudioFocusChange: $focusChange")
    when (focusChange) {
      AudioManager.AUDIOFOCUS_LOSS -> {
        playOnAudioFocus = false
        val body =
          HashMap<String, Any>().apply {
            put("value", "began")
            put("shouldResume", false)
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
      }
      AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
        playOnAudioFocus = lockScreenManager.get()?.isPlaying == true
        val body =
          HashMap<String, Any>().apply {
            put("value", "began")
            put("shouldResume", playOnAudioFocus)
          }
        audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
      }
      AudioManager.AUDIOFOCUS_GAIN -> {
        if (playOnAudioFocus) {
          val body =
            HashMap<String, Any>().apply {
              put("value", "ended")
              put("shouldResume", true)
            }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
        } else {
          val body =
            HashMap<String, Any>().apply {
              put("value", "ended")
              put("shouldResume", false)
            }
          audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody("interruption", body)
        }

        playOnAudioFocus = false
      }
    }
  }

  fun requestAudioFocus(
    focusRequestBuilder: AudioFocusRequest.Builder,
    audioAttributesBuilder: AudioAttributes.Builder,
    legacyStreamType: Int,
    focusGain: Int,
  ): Int =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val focusRequest =
        focusRequestBuilder
          .setFocusGain(focusGain)
          .setAudioAttributes(
            audioAttributesBuilder
              .setLegacyStreamType(legacyStreamType)
              .build(),
          ).build()
      audioManager.requestAudioFocus(focusRequest)
    } else {
      audioManager.requestAudioFocus(this, legacyStreamType, focusGain)
    }

  fun abandonAudioFocus() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && this.focusRequest != null) {
      audioManager.get()?.abandonAudioFocusRequest(focusRequest!!)
    } else {
      audioManager.get()?.abandonAudioFocus(this)
    }
  }
}

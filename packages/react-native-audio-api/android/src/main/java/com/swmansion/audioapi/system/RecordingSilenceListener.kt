package com.swmansion.audioapi.system

import android.media.AudioManager
import android.media.AudioRecordingConfiguration
import android.os.Build
import android.os.Handler
import androidx.annotation.RequiresApi
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference
import java.util.HashMap
import java.util.concurrent.Executor

/**
 * Emits `interruption` when the system silences the active recorder's capture (a call, the
 * Assistant, another recorder, the mic privacy toggle) and when it lets go, without requesting
 * audio focus. Requires API 29; a no-op below. State is written on the main looper only.
 *
 * The recorder's configuration is the one whose client audio session id matches the open capture
 * stream, read on every callback because a disconnect reopens the stream with a new id. A callback
 * without that configuration leaves the state as it was: missing is unknown, not unsilenced.
 *
 * `ended` is held while the audio mode is a call mode other than the one at `began`, so a recording
 * is not resumed mid-call. A system call silences capture while still ringing, so its call mode
 * starts after `began`; a communication mode the host app had already set does not hold `ended`.
 */
class RecordingSilenceListener(
  private val audioManager: AudioManager,
  private val audioAPIModule: WeakReference<AudioAPIModule>,
  private val mainHandler: Handler,
) {
  private var isEnabled = false
  private var isReleased = false
  private var recordingCallback: AudioManager.AudioRecordingCallback? = null

  @Volatile
  private var isCaptureSilenced = false

  /** The audio mode when `began` was emitted; null while no interruption is in progress. */
  @Volatile
  private var modeAtBegan: Int? = null

  private var isAwaitingCallEnd = false

  // AudioManager.OnModeChangedListener is API 31+; held untyped so API 29–30 never resolve it.
  private var modeChangedListener: Any? = null
  private val modePoll =
    object : Runnable {
      override fun run() {
        emitEndedUnlessHeld()
        if (isAwaitingCallEnd) {
          mainHandler.postDelayed(this, MODE_POLL_INTERVAL_MS)
        }
      }
    }

  fun setEnabled(enabled: Boolean) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
      return
    }
    mainHandler.post {
      if (isReleased || enabled == isEnabled) {
        return@post
      }
      isEnabled = enabled
      if (enabled) register() else unregister()
    }
  }

  /** Stops observation for good, without emitting; called when the module is invalidated. */
  fun release() {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
      return
    }
    mainHandler.post {
      isReleased = true
      isEnabled = false
      unregister()
    }
  }

  fun isRecordingSilenced(): Boolean {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q || NativeRecorderControl.captureSessionId() == 0) {
      return false
    }
    return isCaptureSilenced || isEndedHeld()
  }

  @RequiresApi(Build.VERSION_CODES.Q)
  private fun register() {
    val callback =
      object : AudioManager.AudioRecordingCallback() {
        override fun onRecordingConfigChanged(configs: MutableList<AudioRecordingConfiguration>) {
          onRecordingConfigurations(configs)
        }
      }
    recordingCallback = callback
    audioManager.registerAudioRecordingCallback(callback, mainHandler)
    onRecordingConfigurations(audioManager.activeRecordingConfigurations)
  }

  @RequiresApi(Build.VERSION_CODES.Q)
  private fun unregister() {
    recordingCallback?.let { audioManager.unregisterAudioRecordingCallback(it) }
    recordingCallback = null
    reset()
  }

  @RequiresApi(Build.VERSION_CODES.Q)
  private fun onRecordingConfigurations(configs: List<AudioRecordingConfiguration>) {
    val sessionId = NativeRecorderControl.captureSessionId()
    if (sessionId == 0) {
      reset()
      return
    }
    val config = configs.firstOrNull { it.clientAudioSessionId == sessionId } ?: return

    isCaptureSilenced = config.isClientSilenced
    if (!isCaptureSilenced) {
      emitEndedUnlessHeld()
      return
    }
    stopAwaitingCallEnd()
    if (modeAtBegan == null) {
      modeAtBegan = audioManager.mode
      emit("began", shouldResume = false)
    }
  }

  private fun emitEndedUnlessHeld() {
    if (modeAtBegan == null || isCaptureSilenced) {
      stopAwaitingCallEnd()
      return
    }
    if (isEndedHeld()) {
      startAwaitingCallEnd()
      return
    }
    stopAwaitingCallEnd()
    modeAtBegan = null
    emit("ended", shouldResume = true)
  }

  private fun isEndedHeld(): Boolean {
    val began = modeAtBegan ?: return false
    val mode = audioManager.mode
    return isCallMode(mode) && mode != began
  }

  private fun isCallMode(mode: Int): Boolean =
    mode == AudioManager.MODE_IN_CALL ||
      mode == AudioManager.MODE_IN_COMMUNICATION ||
      (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && mode == AudioManager.MODE_CALL_SCREENING)

  private fun startAwaitingCallEnd() {
    if (isAwaitingCallEnd) {
      return
    }
    isAwaitingCallEnd = true
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      addModeChangedListener()
      // The mode may have changed between the last read and the registration.
      mainHandler.post { emitEndedUnlessHeld() }
    } else {
      mainHandler.postDelayed(modePoll, MODE_POLL_INTERVAL_MS)
    }
  }

  private fun stopAwaitingCallEnd() {
    if (!isAwaitingCallEnd) {
      return
    }
    isAwaitingCallEnd = false
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      removeModeChangedListener()
    } else {
      mainHandler.removeCallbacks(modePoll)
    }
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun addModeChangedListener() {
    val listener = AudioManager.OnModeChangedListener { emitEndedUnlessHeld() }
    modeChangedListener = listener
    audioManager.addOnModeChangedListener(Executor { mainHandler.post(it) }, listener)
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun removeModeChangedListener() {
    (modeChangedListener as? AudioManager.OnModeChangedListener)?.let {
      audioManager.removeOnModeChangedListener(it)
    }
    modeChangedListener = null
  }

  private fun reset() {
    stopAwaitingCallEnd()
    modeAtBegan = null
    isCaptureSilenced = false
  }

  private fun emit(
    type: String,
    shouldResume: Boolean,
  ) {
    val body =
      HashMap<String, Any>().apply {
        put("type", type)
        put("shouldResume", shouldResume)
      }
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.INTERRUPTION.ordinal, body)
  }

  companion object {
    private const val MODE_POLL_INTERVAL_MS = 1_000L
  }
}

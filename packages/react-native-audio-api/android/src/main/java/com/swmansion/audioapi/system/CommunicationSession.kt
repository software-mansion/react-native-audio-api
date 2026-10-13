package com.swmansion.audioapi.system

import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Build
import android.os.Handler
import android.util.Log
import androidx.annotation.RequiresApi

/**
 * Android's voice-communication audio session: transient voice-communication focus,
 * `MODE_IN_COMMUNICATION`, the requested communication device, and the route-change callbacks
 * that are live while the session is active. Every method runs on the main thread.
 *
 * Methods that need Android 12 (API 31) refuse or no-op below it, so callers need no version checks.
 */
class CommunicationSession(
  private val audioManager: AudioManager,
  private val audioFocusListener: AudioFocusListener,
  private val mainHandler: Handler,
  private val onRouteChange: (String) -> Unit,
) {
  private var mode: String? = null
  private var preferredDevice: String? = null
  private var previousAudioMode: Int? = null

  // Android keeps a requested communication device until it is cleared or the
  // process dies, with or without a communication session.
  private var deviceRequested = false

  private var deviceCallbacks: CommunicationDeviceCallbacks? = null

  var isActive = false
    private set

  fun configure(
    mode: String?,
    preferredDevice: String?,
  ) {
    this.mode = mode?.takeIf { it.isNotEmpty() }
    this.preferredDevice = preferredDevice?.takeIf { it.isNotEmpty() }
  }

  /**
   * Null on success, otherwise why Android refused. A no-op without `inCommunication` mode
   * and when the session is already active.
   */
  fun activate(): String? {
    if (mode != "inCommunication") {
      return null
    }
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
      return "Android communication sessions require Android 12 (API 31) or later"
    }
    return activateOnApi31()
  }

  /** Releases focus, mode, callbacks, and the device request; safe to call when nothing is held. */
  fun deactivate() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      deactivateOnApi31()
    }
  }

  /** Null on success, otherwise why Android refused the route. */
  fun requestDevice(device: String): String? {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
      return "Android communication devices require Android 12 (API 31) or later"
    }
    return requestDeviceOnApi31(device)
  }

  fun currentDevice(): AudioDeviceInfo? {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
      return null
    }
    return audioManager.communicationDevice
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun activateOnApi31(): String? {
    if (isActive) {
      return null
    }

    previousAudioMode = audioManager.mode
    if (!audioFocusListener.requestCommunicationAudioFocus()) {
      previousAudioMode = null
      return "Android denied voice-communication audio focus"
    }

    return try {
      audioManager.mode = AudioManager.MODE_IN_COMMUNICATION
      deviceCallbacks =
        CommunicationDeviceCallbacks(audioManager, mainHandler, ::forwardRouteChangeWhileActive).also { it.register() }
      preferredDevice?.let { device ->
        requestDeviceOnApi31(device)?.let { error -> throw IllegalStateException(error) }
      }
      isActive = true
      null
    } catch (error: Exception) {
      deactivateOnApi31()
      error.message ?: "Could not activate Android communication audio session"
    }
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun deactivateOnApi31() {
    val ownsSession = isActive || previousAudioMode != null
    if (!ownsSession && !deviceRequested) {
      return
    }

    isActive = false
    val callbacks = deviceCallbacks
    deviceCallbacks = null
    if (callbacks != null) {
      try {
        callbacks.unregister()
      } catch (error: Exception) {
        Log.w("CommunicationSession", "Could not unregister communication-device callbacks", error)
      }
    }
    audioManager.clearCommunicationDevice()
    deviceRequested = false
    if (ownsSession) {
      audioFocusListener.abandonCommunicationAudioFocus()
      previousAudioMode?.let { audioManager.mode = it }
      previousAudioMode = null
    }
  }

  @RequiresApi(Build.VERSION_CODES.S)
  private fun requestDeviceOnApi31(device: String): String? {
    if (device == "systemDefault") {
      audioManager.clearCommunicationDevice()
      deviceRequested = false
      return null
    }

    val deviceType =
      when (device) {
        "speaker" -> AudioDeviceInfo.TYPE_BUILTIN_SPEAKER
        "earpiece" -> AudioDeviceInfo.TYPE_BUILTIN_EARPIECE
        else -> return "Unsupported communication device: $device"
      }
    val target =
      audioManager.availableCommunicationDevices.firstOrNull { it.type == deviceType }
        ?: return "Requested communication device is unavailable: $device"

    if (!audioManager.setCommunicationDevice(target)) {
      return "Android rejected requested communication device: $device"
    }
    deviceRequested = true
    return null
  }

  // A callback can still land on the main thread after a failed activation tore the session down.
  private fun forwardRouteChangeWhileActive(reason: String) {
    if (isActive) {
      onRouteChange(reason)
    }
  }
}

package com.swmansion.audioapi.system

import android.media.AudioDeviceCallback
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Build
import android.os.Handler
import androidx.annotation.RequiresApi
import java.util.concurrent.Executor

@RequiresApi(Build.VERSION_CODES.S)
fun registerCommunicationDeviceCallbacks(
  audioManager: AudioManager,
  mainHandler: Handler,
  onRouteChange: (String) -> Unit,
): Any = CommunicationDeviceCallbacks.register(audioManager, mainHandler, onRouteChange)

@RequiresApi(Build.VERSION_CODES.S)
fun unregisterCommunicationDeviceCallbacks(callbacks: Any) {
  CommunicationDeviceCallbacks.unregister(callbacks)
}

@RequiresApi(Build.VERSION_CODES.S)
private object CommunicationDeviceCallbacks {
  fun register(
    audioManager: AudioManager,
    mainHandler: Handler,
    onRouteChange: (String) -> Unit,
  ): Any = Registration(audioManager, mainHandler, onRouteChange).also { it.register() }

  fun unregister(callbacks: Any) {
    (callbacks as Registration).unregister()
  }

  private class Registration(
    private val audioManager: AudioManager,
    private val mainHandler: Handler,
    private val onRouteChange: (String) -> Unit,
  ) {
    private val deviceChangedListener =
      AudioManager.OnCommunicationDeviceChangedListener {
        onRouteChange("Override")
      }
    private val deviceCallback =
      object : AudioDeviceCallback() {
        override fun onAudioDevicesAdded(addedDevices: Array<AudioDeviceInfo>) {
          onRouteChange("NewDeviceAvailable")
        }

        override fun onAudioDevicesRemoved(removedDevices: Array<AudioDeviceInfo>) {
          onRouteChange("OldDeviceUnavailable")
        }
      }
    private val mainExecutor = Executor { command -> mainHandler.post(command) }
    private var deviceChangedListenerRegistered = false
    private var audioDeviceCallbackRegistered = false

    fun register() {
      try {
        audioManager.addOnCommunicationDeviceChangedListener(mainExecutor, deviceChangedListener)
        deviceChangedListenerRegistered = true
        audioManager.registerAudioDeviceCallback(deviceCallback, mainHandler)
        audioDeviceCallbackRegistered = true
      } catch (error: Exception) {
        try {
          unregister()
        } catch (_: Exception) {
          // The activation transaction still reports its original failure.
        }
        throw error
      }
    }

    fun unregister() {
      var failure: Exception? = null
      if (deviceChangedListenerRegistered) {
        deviceChangedListenerRegistered = false
        try {
          audioManager.removeOnCommunicationDeviceChangedListener(deviceChangedListener)
        } catch (error: Exception) {
          failure = error
        }
      }
      if (audioDeviceCallbackRegistered) {
        audioDeviceCallbackRegistered = false
        try {
          audioManager.unregisterAudioDeviceCallback(deviceCallback)
        } catch (error: Exception) {
          if (failure == null) {
            failure = error
          }
        }
      }
      if (failure != null) {
        throw failure
      }
    }
  }
}

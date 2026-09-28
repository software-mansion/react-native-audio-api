package com.swmansion.audioapi.system

import android.media.AudioDeviceCallback
import android.media.AudioDeviceInfo
import com.swmansion.audioapi.AudioAPIModule
import java.lang.ref.WeakReference
import java.util.HashMap

class DeviceChangeListener(
  private val audioAPIModule: WeakReference<AudioAPIModule>,
) : AudioDeviceCallback() {
  /** Registration replays every connected device as added; that snapshot is not a change. */
  private var receivedInitialDevices = false

  override fun onAudioDevicesAdded(addedDevices: Array<out AudioDeviceInfo>) {
    if (!receivedInitialDevices) {
      receivedInitialDevices = true
      return
    }

    dispatchRouteChange("NewDeviceAvailable")
  }

  override fun onAudioDevicesRemoved(removedDevices: Array<out AudioDeviceInfo>) {
    dispatchRouteChange("OldDeviceUnavailable")
  }

  private fun dispatchRouteChange(reason: String) {
    val body = HashMap<String, Any>().apply { put("reason", reason) }
    audioAPIModule.get()?.invokeHandlerWithEventNameAndEventBody(AudioEvent.ROUTE_CHANGE.ordinal, body)
  }
}

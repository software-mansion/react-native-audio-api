package com.swmansion.audioapi.system

import android.media.AudioDeviceCallback
import android.media.AudioDeviceInfo

class DeviceChangeListener : AudioDeviceCallback() {
  /** Registration replays every connected device as added; that snapshot is not a change. */
  private var receivedInitialDevices = false

  override fun onAudioDevicesAdded(addedDevices: Array<out AudioDeviceInfo>) {
    if (!receivedInitialDevices) {
      receivedInitialDevices = true
      return
    }

    MediaSessionManager.inputRouter.followAddedInputDevices(addedDevices)
    MediaSessionManager.dispatchRouteChange(RouteChangeReason.NEW_DEVICE_AVAILABLE)
  }

  override fun onAudioDevicesRemoved(removedDevices: Array<out AudioDeviceInfo>) {
    MediaSessionManager.inputRouter.forgetPreferredInputDeviceIfRemoved(removedDevices)
    MediaSessionManager.dispatchRouteChange(RouteChangeReason.OLD_DEVICE_UNAVAILABLE)
  }
}

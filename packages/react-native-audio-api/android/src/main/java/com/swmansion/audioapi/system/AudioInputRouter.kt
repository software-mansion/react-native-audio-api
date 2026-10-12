package com.swmansion.audioapi.system

import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Build
import androidx.annotation.RequiresApi

/**
 * Decides which input device a recording captures from and brings up the routing that
 * device needs.
 *
 * The choice is either the device selected through `setInputDevice`, or, when nothing is
 * selected, the external mic the system should prefer. Recorders take their device from
 * [acquireInputRoute] and give it back through [releaseInputRoute].
 */
class AudioInputRouter(
  private val audioManager: AudioManager,
  private val bluetoothScoLink: BluetoothScoLink,
) {
  /** Guards [preferredInputDeviceId] and [acquiredInputRouteCount]. */
  private val inputSelectionLock = Any()

  /**
   * Null means the platform picks the capture device.
   *
   * Written under [inputSelectionLock]; volatile so it can be read without.
   */
  @Volatile
  var preferredInputDeviceId: Int? = null
    private set

  /**
   * Routes acquired and not yet released. Only one recording runs at a time, but a restart
   * can acquire its new route while a recording started in the meantime holds one too, so
   * this is a count and not a flag. Guarded by [inputSelectionLock].
   */
  private var acquiredInputRouteCount = 0

  /** The device the latest [acquireInputRoute] handed out, null for the system default. */
  @Volatile
  private var captureDeviceId: Int? = null

  /** What [acquireInputRoute] hands a recorder. */
  class InputRoute(
    /** Device to open the capture stream on, or null for the system default. */
    val deviceId: Int?,
    /** False when the routing that device needs could not be set up. */
    val routed: Boolean,
  )

  @RequiresApi(Build.VERSION_CODES.M)
  fun findInputDevice(deviceId: String): AudioDeviceInfo? = recordableInputs().firstOrNull { it.id.toString() == deviceId }

  /** Connected inputs a recording can open, without the ones reserved for the system. */
  @RequiresApi(Build.VERSION_CODES.M)
  fun recordableInputs(): List<AudioDeviceInfo> =
    audioManager
      .getDevices(AudioManager.GET_DEVICES_INPUTS)
      .filter { it.type !in systemOnlyInputTypes }

  private val systemOnlyInputTypes =
    setOf(
      AudioDeviceInfo.TYPE_TELEPHONY,
      AudioDeviceInfo.TYPE_REMOTE_SUBMIX,
      AudioDeviceInfo.TYPE_FM_TUNER,
      AudioDeviceInfo.TYPE_TV_TUNER,
    )

  /**
   * A running or paused recording is moved onto the new choice before this returns, which
   * blocks while its stream reopens; never call it on the main thread.
   *
   * @param device null hands the choice back to the system.
   * @return false when a recording was running and could not continue on the new input,
   *   in which case the previous selection stays in place.
   */
  fun setPreferredInputDevice(device: AudioDeviceInfo?): Boolean {
    val previousInputDeviceId =
      synchronized(inputSelectionLock) {
        if (device?.id == preferredInputDeviceId) {
          return true
        }

        val previous = preferredInputDeviceId
        preferredInputDeviceId = device?.id
        previous
      }

    // Does nothing when no recording is in progress.
    if (NativeInputRouting.rerouteActiveCapture()) {
      return true
    }

    // The recording stopped instead of moving, so a selection that never took effect is not
    // kept for the next one. A newer selection made meanwhile wins.
    synchronized(inputSelectionLock) {
      if (preferredInputDeviceId == device?.id) {
        preferredInputDeviceId = previousInputDeviceId
      }
    }

    return false
  }

  /**
   * Moves a recording in progress onto an external mic that just connected, when the input
   * is left to the system and that mic is now the one [automaticInput] prefers.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  fun followAddedInputDevices(addedDevices: Array<out AudioDeviceInfo>) {
    val followsSystemInput = preferredInputDeviceId == null

    if (!followsSystemInput || addedDevices.none { it.isSource && it.type in automaticInputTypes }) {
      return
    }

    val preferredInput = automaticInput()

    if (preferredInput == null || preferredInput.id == captureDeviceId) {
      return
    }

    // Off the calling (main) thread: rerouting can wait for a Bluetooth headset's link.
    // Does nothing when no recording is in progress.
    Thread { NativeInputRouting.rerouteActiveCapture() }.start()
  }

  /**
   * Tells a recorder about to open its capture stream which device to open it on, after
   * bringing up the routing that device needs and waiting for it.
   *
   * Blocks while a Bluetooth headset's link comes up, so never call it on the main thread.
   * Every call must be balanced by [releaseInputRoute], also when the route reports `routed = false`.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  fun acquireInputRoute(): InputRoute {
    val selectedInputDeviceId =
      synchronized(inputSelectionLock) {
        acquiredInputRouteCount++
        preferredInputDeviceId
      }

    // Outside the lock, so the wait cannot stall the device-removal callback on the main thread.
    if (selectedInputDeviceId != null) {
      val selectedInput = recordableInputs().firstOrNull { it.id == selectedInputDeviceId }
      val routed = selectedInput == null || prepareCaptureRouting(selectedInput)
      captureDeviceId = selectedInputDeviceId
      return InputRoute(selectedInputDeviceId, routed)
    }

    captureDeviceId = automaticInputId()
    return InputRoute(captureDeviceId, routed = true)
  }

  /** Gives back a route from [acquireInputRoute]; the last one tears the routing down. */
  fun releaseInputRoute() {
    val wasLastCapture =
      synchronized(inputSelectionLock) {
        acquiredInputRouteCount = maxOf(0, acquiredInputRouteCount - 1)
        acquiredInputRouteCount == 0
      }

    if (wasLastCapture) {
      bluetoothScoLink.tearDown()
    }
  }

  /**
   * Returns the selection to the system default when the selected device is among
   * [removedDevices]. A reconnected device gets a new id, so the old one would make
   * every later capture stream ask for a device that no longer exists.
   */
  fun forgetPreferredInputDeviceIfRemoved(removedDevices: Array<out AudioDeviceInfo>) {
    synchronized(inputSelectionLock) {
      val selectedInputDeviceId = preferredInputDeviceId ?: return

      if (removedDevices.none { it.id == selectedInputDeviceId }) {
        return
      }

      // Cleared even while a capture runs: the disconnect is tearing that stream down anyway.
      preferredInputDeviceId = null
    }

    bluetoothScoLink.tearDown()
  }

  /** Tears down any routing still up, for when the module goes away mid-recording. */
  fun cleanup() {
    bluetoothScoLink.tearDown()
  }

  /**
   * External mics a recording uses when nothing is selected, most preferred first. A
   * dedicated USB mic or interface is the clearest sign of intent; a Bluetooth headset
   * comes last because its mic is call quality and costs the headset its playback quality.
   */
  private val automaticInputTypes =
    listOf(
      AudioDeviceInfo.TYPE_USB_DEVICE,
      AudioDeviceInfo.TYPE_USB_HEADSET,
      AudioDeviceInfo.TYPE_WIRED_HEADSET,
      AudioDeviceInfo.TYPE_BLUETOOTH_SCO,
    )

  /** The connected external mic to capture from when nothing is selected, if any. */
  @RequiresApi(Build.VERSION_CODES.M)
  private fun automaticInput(): AudioDeviceInfo? {
    val inputs = recordableInputs()

    return automaticInputTypes.firstNotNullOfOrNull { type -> inputs.firstOrNull { it.type == type } }
  }

  /**
   * The id of [automaticInput], or null to leave the choice to the platform.
   *
   * Unlike an explicit selection, a headset whose link does not come up is not an error:
   * the capture falls back to the platform's default input.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  private fun automaticInputId(): Int? {
    val input = automaticInput() ?: return null

    if (!prepareCaptureRouting(input)) {
      bluetoothScoLink.tearDown()
      return null
    }

    return input.id
  }

  @RequiresApi(Build.VERSION_CODES.M)
  private fun prepareCaptureRouting(input: AudioDeviceInfo): Boolean {
    if (input.type != AudioDeviceInfo.TYPE_BLUETOOTH_SCO) {
      return true
    }

    return bluetoothScoLink.bringUp(input)
  }
}

package com.swmansion.audioapi.system

import android.os.Build
import android.util.Log

/** Both directions of the input-routing bridge to the native recorder, see `NativeInputRouting.h`. */
object NativeInputRouting {
  private const val TAG = "NativeInputRouting"

  init {
    System.loadLibrary("react-native-audio-api")
  }

  /**
   * Moves the active recording, if any, onto the input [MediaSessionManager.acquireInputRoute]
   * now hands out. Blocks while the stream reopens; never call it on the main thread.
   *
   * @return false when the recorder could not continue on the new input.
   */
  @JvmStatic
  external fun rerouteActiveCapture(): Boolean

  /** Oboe's `kUnspecified`: the platform picks the capture device. */
  private const val SYSTEM_DEFAULT_DEVICE_ID = 0

  /** Mirrors `kRoutingFailed` on the native side; never a valid device id. */
  private const val ROUTING_FAILED = -1

  /**
   * @return the id of the device the capture stream should open on,
   * [SYSTEM_DEFAULT_DEVICE_ID] to let the platform pick, or [ROUTING_FAILED].
   */
  @JvmStatic
  fun acquireInputRoute(): Int {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
      return SYSTEM_DEFAULT_DEVICE_ID
    }

    return try {
      val route = MediaSessionManager.acquireInputRoute()

      if (route.routed) route.deviceId ?: SYSTEM_DEFAULT_DEVICE_ID else ROUTING_FAILED
    } catch (exception: RuntimeException) {
      Log.w(TAG, "Could not prepare the input route", exception)
      ROUTING_FAILED
    }
  }

  @JvmStatic
  fun releaseInputRoute() {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
      return
    }

    try {
      MediaSessionManager.releaseInputRoute()
    } catch (exception: RuntimeException) {
      Log.w(TAG, "Could not release the input route", exception)
    }
  }
}

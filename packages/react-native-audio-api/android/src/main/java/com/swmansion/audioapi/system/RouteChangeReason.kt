package com.swmansion.audioapi.system

/**
 * Why a `routeChange` event fired. [eventName] is the value JS receives and mirrors the
 * `RouteChangeReason` union in `src/events/types.ts`.
 */
enum class RouteChangeReason(
  val eventName: String,
) {
  NEW_DEVICE_AVAILABLE("NewDeviceAvailable"),
  OLD_DEVICE_UNAVAILABLE("OldDeviceUnavailable"),

  /** The device a recording captures from changed, or the recording stopped. */
  CONFIGURATION_CHANGE("ConfigurationChange"),
}

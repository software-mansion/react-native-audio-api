package com.swmansion.audioapi.system

import android.content.Intent
import android.os.Build
import androidx.core.app.NotificationManagerCompat
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.modules.core.DeviceEventManagerModule

class MediaSessionEventEmitter(
  val reactContext: ReactApplicationContext,
  val notificationId: Int,
) {
  fun sendEvent(
    name: String,
    values: Map<String, Any>?,
  ) {
    val data = Arguments.createMap()

    if (values != null) {
      for (value in values) {
        when (value.value) {
          is Double, is Float -> {
            data.putDouble(value.key, value.value as Double)
          }

          is Boolean -> {
            data.putBoolean(value.key, value.value as Boolean)
          }

          is Int -> {
            data.putInt(value.key, value.value as Int)
          }
        }
      }
    }

    reactContext
      .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
      .emit(name, data)
  }

  fun onPlay() {
    sendEvent("onRemotePlay", null)
  }

  fun onPause() {
    sendEvent("onRemotePause", null)
  }

  fun onStop() {
    stopForegroundService()
    sendEvent("onRemoteStop", null)
  }

  fun onSkipToNext() {
    sendEvent("onRemoteNextTrack", null)
  }

  fun onSkipToPrevious() {
    sendEvent("onRemotePreviousTrack", null)
  }

  fun onFastForward() {
    sendEvent("onRemoteSkipForward", null)
  }

  fun onRewind() {
    sendEvent("onRemoteSkipBackward", null)
  }

  fun onInterruption(values: Map<String, Any>) {
    sendEvent("onInterruption", values)
  }

  fun onVolumeChange(values: Map<String, Number>) {
    sendEvent("onVolumeChange", values)
  }

  private fun stopForegroundService() {
    NotificationManagerCompat.from(reactContext).cancel(notificationId)
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val myIntent =
        Intent(reactContext, MediaNotificationManager.NotificationService::class.java)
      reactContext.stopService(myIntent)
    }
  }
}

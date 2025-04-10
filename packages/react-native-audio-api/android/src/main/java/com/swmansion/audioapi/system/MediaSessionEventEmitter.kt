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
    value: Any?,
    dataName: String?,
  ) {
    val data = Arguments.createMap()

    if (value != null && dataName != null) {
      when (value) {
        is Double, is Float -> {
          data.putDouble(dataName, value as Double)
        }

        is Boolean -> {
          data.putBoolean(dataName, value)
        }

        is Int -> {
          data.putInt(dataName, value)
        }
      }
    }

    reactContext
      .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
      .emit(name, data)
  }

  fun onPlay() {
    sendEvent("onRemotePlay", null, null)
  }

  fun onPause() {
    sendEvent("onRemotePause", null, null)
  }

  fun onStop() {
    stopForegroundService()
    sendEvent("onRemoteStop", null, null)
  }

  fun onSkipToNext() {
    sendEvent("onRemoteNextTrack", null, null)
  }

  fun onSkipToPrevious() {
    sendEvent("onRemotePreviousTrack", null, null)
  }

  fun onFastForward() {
    sendEvent("onRemoteSkipForward", null, null)
  }

  fun onRewind() {
    sendEvent("onRemoteSkipBackward", null, null)
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

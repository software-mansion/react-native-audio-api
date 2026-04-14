package com.swmansion.audioapi.system

import android.content.pm.PackageManager
import com.facebook.react.bridge.Promise
import com.facebook.react.modules.core.PermissionListener

class PermissionRequestListener(
  private val promise: Promise,
) : PermissionListener {
  companion object {
    const val RECORDING_REQUEST_CODE = 1234
    const val NOTIFICATION_REQUEST_CODE = 1235
  }

  override fun onRequestPermissionsResult(
    requestCode: Int,
    permissions: Array<String>,
    grantResults: IntArray,
  ): Boolean {
    if (requestCode == RECORDING_REQUEST_CODE || requestCode == NOTIFICATION_REQUEST_CODE) {
      if (grantResults.isEmpty()) {
        this.promise.resolve("Undetermined")
      } else {
        val granted = grantResults[0] == PackageManager.PERMISSION_GRANTED
        if (granted) {
          this.promise.resolve("Granted")
        } else {
          this.promise.resolve("Denied")
        }
      }
    }
    return true
  }
}

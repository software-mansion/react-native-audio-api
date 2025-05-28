// com/swmansion/audioapi/system/RecordingPermissionActivity.kt
package com.swmansion.audioapi.system

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class RecordingPermissionActivity : Activity() {
  companion object {
    const val REQUEST_CODE = 1234
    var onResult: ((info: String) -> Unit)? = null
  }

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
      == PackageManager.PERMISSION_GRANTED
    ) {
      onResult?.invoke("Granted")
      finish()
    } else {
      ActivityCompat.requestPermissions(
        this,
        arrayOf(Manifest.permission.RECORD_AUDIO),
        REQUEST_CODE,
      )
    }
  }

  override fun onRequestPermissionsResult(
    requestCode: Int,
    permissions: Array<out String>,
    grantResults: IntArray,
  ) {
    if (requestCode == REQUEST_CODE) {
      if (grantResults.isEmpty()) {
        onResult?.invoke("Undetermined")
      } else {
        val granted = grantResults[0] == PackageManager.PERMISSION_GRANTED
        if (granted) {
          onResult?.invoke("Granted")
        } else {
          onResult?.invoke("Denied")
        }
      }
    }
    finish()
  }
}

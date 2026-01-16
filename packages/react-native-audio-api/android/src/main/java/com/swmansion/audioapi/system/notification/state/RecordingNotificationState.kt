package com.swmansion.audioapi.system.notification.state

import android.content.Intent
import androidx.core.app.NotificationCompat
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.system.notification.RecordingNotificationReceiver

data class RecordingNotificationState(
  var builder: NotificationCompat.Builder? = null,
  var receiver: RecordingNotificationReceiver? = null,
  var initialized: Boolean,
  var pauseIntent: Intent? = null,
  var resumeIntent: Intent? = null,
  var title: String? = null,
  var contentText: String? = null,
  var paused: Boolean = false,
  var smallIconResourceName: String? = null,
  var largeIconResourceName: String? = null,
  var pauseIconResourceName: String? = null,
  var resumeIconResourceName: String? = null,
  var backgroundColor: Int? = null,
  var cachedRNOptions: ReadableMap? = null,
  var darkTheme: Boolean,
)

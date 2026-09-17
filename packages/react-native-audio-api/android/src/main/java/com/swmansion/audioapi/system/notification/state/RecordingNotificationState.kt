package com.swmansion.audioapi.system.notification.state

import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.system.notification.RecordingNotificationReceiver

/**
 * Options are sticky: a `show()` call keeps every value the previous call set unless the
 * new options override it.
 */
class RecordingNotificationState(
  var receiver: RecordingNotificationReceiver? = null,
  var initialized: Boolean = false,
  /** Last options map handed to `show()`, so an unchanged one can skip re-parsing. */
  var cachedRNOptions: ReadableMap? = null,
  var title: String? = null,
  var contentText: String? = null,
  var paused: Boolean = false,
  var smallIconResourceName: String? = null,
  var largeIconResourceName: String? = null,
  var backgroundColor: Int? = null,
  var showStopAction: Boolean = false,
  var dismissible: Boolean = false,
  var pauseActionTitle: String? = null,
  var resumeActionTitle: String? = null,
  var stopActionTitle: String? = null,
  var deepLinkUri: String? = null,
  var usesChronometer: Boolean = false,
  var startedAtMs: Long? = null,
  var pausedAtMs: Long? = null,
)

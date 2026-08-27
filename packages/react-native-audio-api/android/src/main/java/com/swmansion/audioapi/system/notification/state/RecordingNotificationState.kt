package com.swmansion.audioapi.system.notification.state

import com.swmansion.audioapi.system.notification.RecordingNotificationReceiver

/**
 * Options are sticky: a `show()` call keeps every value the previous call set unless the
 * new options override it. The only exception is `paused`, which resets to `false` when
 * absent so the notification never sticks in the paused look.
 */
class RecordingNotificationState(
  var receiver: RecordingNotificationReceiver? = null,
  var initialized: Boolean = false,
  var title: String? = null,
  var contentText: String? = null,
  var paused: Boolean = false,
  var smallIconResourceName: String? = null,
  var largeIconResourceName: String? = null,
  var backgroundColor: Int? = null,
  var showStopAction: Boolean = false,
  var pauseActionTitle: String? = null,
  var resumeActionTitle: String? = null,
  var stopActionTitle: String? = null,
  var deepLinkUri: String? = null,
  var usesChronometer: Boolean = false,
  var startedAtMs: Long? = null,
  var pausedAtMs: Long? = null,
)

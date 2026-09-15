package com.swmansion.audioapi.system.notification.state

import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.system.notification.RecordingNotificationReceiver

/**
 * Options are sticky: a `show()` call keeps every value the previous call set unless the
 * new options override it. That includes `paused`, which the notification's own pause and
 * resume actions write from the recorder's state — a partial `show()` omitting it must not
 * contradict them. Everything belonging to a single recording session, `paused` included,
 * is cleared when the notification is hidden.
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
  var pauseActionTitle: String? = null,
  var resumeActionTitle: String? = null,
  var stopActionTitle: String? = null,
  var deepLinkUri: String? = null,
  var usesChronometer: Boolean = false,
  var startedAtMs: Long? = null,
  var pausedAtMs: Long? = null,
)

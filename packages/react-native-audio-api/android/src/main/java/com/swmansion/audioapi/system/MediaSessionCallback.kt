package com.swmansion.audioapi.system

import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat

class MediaSessionCallback(
  val eventEmitter: MediaSessionEventEmitter,
  private val lockScreenManager: LockScreenManager,
) : MediaSessionCompat.Callback() {
  override fun onPlay() {
    lockScreenManager.updatePlaybackState(PlaybackStateCompat.STATE_PLAYING)
    eventEmitter.onPlay()
  }

  override fun onPause() {
    lockScreenManager.updatePlaybackState(PlaybackStateCompat.STATE_PAUSED)
    eventEmitter.onPause()
  }

  override fun onStop() {
    eventEmitter.onStop()
  }

  override fun onSkipToNext() {
    eventEmitter.onSkipToNext()
  }

  override fun onSkipToPrevious() {
    eventEmitter.onSkipToPrevious()
  }

  override fun onFastForward() {
    eventEmitter.onFastForward()
  }

  override fun onRewind() {
    eventEmitter.onRewind()
  }
}

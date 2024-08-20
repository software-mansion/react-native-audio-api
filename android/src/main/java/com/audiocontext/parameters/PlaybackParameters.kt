package com.audiocontext.parameters

import android.media.AudioTrack

class PlaybackParameters(val audioTrack: AudioTrack, var buffer: ShortArray) {
  var leftPan = 1.0
  var rightPan = 1.0

  fun reset() {
    leftPan = 1.0
    rightPan = 1.0
  }
}

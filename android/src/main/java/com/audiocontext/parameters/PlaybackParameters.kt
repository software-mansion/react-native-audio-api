package com.audiocontext.parameters

import android.media.AudioTrack
import com.audiocontext.utils.AudioBuffer

class PlaybackParameters(val audioTrack: AudioTrack, var audioBuffer: AudioBuffer) {
  var leftPan = 1.0
  var rightPan = 1.0

  fun reset() {
    leftPan = 1.0
    rightPan = 1.0
  }
}

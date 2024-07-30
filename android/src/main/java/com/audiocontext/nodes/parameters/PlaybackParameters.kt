package com.audiocontext.nodes.parameters

import android.media.AudioTrack

class PlaybackParameters(val audioTrack: AudioTrack, var buffer: ShortArray) {
  var leftPan = 1.0
  var rightPan = 1.0
  var gain = 1.0
}

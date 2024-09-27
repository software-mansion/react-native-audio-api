package com.swmansion.audioapi.parameters

import android.media.AudioTrack
import com.swmansion.audioapi.utils.AudioBuffer

class PlaybackParameters(
  val audioTrack: AudioTrack,
  var audioBuffer: AudioBuffer,
)

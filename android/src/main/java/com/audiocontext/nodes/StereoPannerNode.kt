package com.audiocontext.nodes

import android.os.Build
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.AudioParam
import com.audiocontext.nodes.parameters.PlaybackParameters
import kotlin.math.min

class StereoPannerNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  private var pan: AudioParam = AudioParam(context,0.0, 1.0, -1.0)
    get() = field
    set(value) {
      field = value
    }

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    playbackParameters.leftPan = min(1.0 - pan.getValueAtTime(context.getCurrentTime()), 1.0)
    playbackParameters.rightPan = min(1.0 + pan.getValueAtTime(context.getCurrentTime()), 1.0)
    super.process(playbackParameters)
  }
}

package com.audiocontext.nodes

import android.os.Build
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.AudioParam
import com.audiocontext.nodes.parameters.PlaybackParameters

class GainNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  private var gain: AudioParam = AudioParam(context,0.5, 1.0, 0.0)
    get() = field
    set(value) {
      field = value
    }

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    playbackParameters.gain = gain.getValueAtTime(context.getCurrentTime())
    super.process(playbackParameters)
  }
}

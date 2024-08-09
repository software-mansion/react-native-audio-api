package com.audiocontext.nodes

import android.os.Build
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.AudioParam
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.Constants

class GainNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  private val gain: AudioParam = AudioParam(context,1.0, Constants.MAX_GAIN, -Constants.MAX_GAIN)
    get() = field

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    playbackParameters.gain = gain.getValueAtTime(context.getCurrentTime())
    super.process(playbackParameters)
  }
}

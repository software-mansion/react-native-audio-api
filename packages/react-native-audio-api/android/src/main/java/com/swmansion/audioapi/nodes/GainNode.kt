package com.swmansion.audioapi.nodes

import android.os.Build
import androidx.annotation.RequiresApi
import com.swmansion.audioapi.context.BaseAudioContext
import com.swmansion.audioapi.nodes.audionode.AudioNode
import com.swmansion.audioapi.parameters.AudioParam
import com.swmansion.audioapi.parameters.PlaybackParameters
import com.swmansion.audioapi.utils.Constants

class GainNode(
  context: BaseAudioContext,
) : AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1

  private val gain: AudioParam = AudioParam(context, 1.0, Constants.MAX_GAIN, -Constants.MAX_GAIN)
    get() = field

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    mixBuffers(playbackParameters)

    for (i in 0 until playbackParameters.audioBuffer.length) {
      val gain = this.gain.getValueAtTime(context.getCurrentTime())

      for (j in 0 until playbackParameters.audioBuffer.numberOfChannels) {
        playbackParameters.audioBuffer.getChannelData(j)[i] = (playbackParameters.audioBuffer.getChannelData(j)[i] * gain).toFloat()
      }
    }

    super.process(playbackParameters)
  }
}

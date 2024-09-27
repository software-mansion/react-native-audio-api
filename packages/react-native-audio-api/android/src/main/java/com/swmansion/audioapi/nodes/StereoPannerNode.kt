package com.swmansion.audioapi.nodes

import android.os.Build
import androidx.annotation.RequiresApi
import com.swmansion.audioapi.context.BaseAudioContext
import com.swmansion.audioapi.nodes.audionode.AudioNode
import com.swmansion.audioapi.nodes.audionode.ChannelCountMode
import com.swmansion.audioapi.parameters.AudioParam
import com.swmansion.audioapi.parameters.PlaybackParameters
import com.swmansion.audioapi.utils.Constants
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.sin

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

class StereoPannerNode(
  context: BaseAudioContext,
) : AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  override val channelCountMode: ChannelCountMode = ChannelCountMode.CLAMPED_MAX

  private val pan: AudioParam = AudioParam(context, 0.0, Constants.MAX_PAN, -Constants.MAX_PAN)
    get() = field

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    mixBuffers(playbackParameters)

    val bufferL = playbackParameters.audioBuffer.getChannelData(0)
    val bufferR = playbackParameters.audioBuffer.getChannelData(1)

    for (i in 0 until playbackParameters.audioBuffer.length) {
      val pan = pan.getValueAtTime(context.getCurrentTime())
      val x = (if (pan <= 0) pan + 1 else pan) * PI / 2

      val gainL = cos(x)
      val gainR = sin(x)

      val inputL = bufferL[i]
      val inputR = bufferR[i]

      if (pan <= 0) {
        bufferL[i] += (inputR * gainL).toFloat()
        bufferR[i] *= (gainR).toFloat()
      } else {
        bufferL[i] *= (gainL).toFloat()
        bufferR[i] += (inputL * gainR).toFloat()
      }
    }

    super.process(playbackParameters)
  }
}

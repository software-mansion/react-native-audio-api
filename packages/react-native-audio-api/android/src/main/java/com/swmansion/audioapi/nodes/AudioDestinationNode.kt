package com.swmansion.audioapi.nodes

import android.media.AudioTrack.WRITE_BLOCKING
import com.swmansion.audioapi.context.BaseAudioContext
import com.swmansion.audioapi.nodes.audionode.AudioNode
import com.swmansion.audioapi.nodes.audionode.ChannelCountMode
import com.swmansion.audioapi.parameters.PlaybackParameters
import kotlin.math.abs
import kotlin.math.max

class AudioDestinationNode(
  context: BaseAudioContext,
) : AudioNode(context) {
  override val numberOfInputs = Float.POSITIVE_INFINITY.toInt()
  override val numberOfOutputs = 0
  override val channelCountMode: ChannelCountMode = ChannelCountMode.EXPLICIT

  private fun normalizeBuffer(playbackParameters: PlaybackParameters) {
    val bufferL = playbackParameters.audioBuffer.getChannelData(0)
    val bufferR = playbackParameters.audioBuffer.getChannelData(1)

    var maxL = 1.0
    var maxR = 1.0

    for (i in 0 until playbackParameters.audioBuffer.length) {
      maxL = max(maxL, abs(bufferL[i].toDouble()))
      maxR = max(maxR, abs(bufferR[i].toDouble()))
    }

    for (i in 0 until playbackParameters.audioBuffer.length) {
      bufferL[i] = (bufferL[i] / maxL).toFloat()
      bufferR[i] = (bufferR[i] / maxR).toFloat()
    }
  }

  override fun process(playbackParameters: PlaybackParameters) {
    mixBuffers(playbackParameters)

    normalizeBuffer(playbackParameters)

    val buffer = FloatArray(playbackParameters.audioBuffer.length * playbackParameters.audioBuffer.numberOfChannels)
    for (i in 0 until playbackParameters.audioBuffer.length) {
      for (j in 0 until playbackParameters.audioBuffer.numberOfChannels) {
        buffer[i * playbackParameters.audioBuffer.numberOfChannels + j] = playbackParameters.audioBuffer.getChannelData(j)[i]
      }
    }

    playbackParameters.audioTrack.write(
      buffer,
      0,
      playbackParameters.audioBuffer.length * playbackParameters.audioBuffer.numberOfChannels,
      WRITE_BLOCKING,
    )
  }
}

package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.audionode.AudioNode
import com.audiocontext.nodes.audionode.ChannelCountMode
import com.audiocontext.parameters.PlaybackParameters


class AudioDestinationNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs = Float.POSITIVE_INFINITY.toInt()
  override val numberOfOutputs = 0
  override val channelCount: Int = 2
  override val channelCountMode: ChannelCountMode = ChannelCountMode.EXPLICIT

  private fun setVolumeAndPanning(playbackParameters: PlaybackParameters) {
    playbackParameters.audioTrack.setStereoVolume(playbackParameters.leftPan.toFloat(), playbackParameters.rightPan.toFloat())
  }

  override fun process(playbackParameters: PlaybackParameters) {
    mixBuffers(playbackParameters)

    setVolumeAndPanning(playbackParameters)
    val buffer = ShortArray(playbackParameters.audioBuffer.length * playbackParameters.audioBuffer.numberOfChannels)
    for (i in 0 until playbackParameters.audioBuffer.length) {
      for (j in 0 until playbackParameters.audioBuffer.numberOfChannels) {
        buffer[i * playbackParameters.audioBuffer.numberOfChannels + j] = playbackParameters.audioBuffer.getChannelData(j)[i]
      }
    }

    playbackParameters.audioTrack.write(buffer, 0, playbackParameters.audioBuffer.length * playbackParameters.audioBuffer.numberOfChannels)
  }
}

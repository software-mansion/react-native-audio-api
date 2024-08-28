package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.audionode.AudioNode
import com.audiocontext.parameters.PlaybackParameters


class AudioDestinationNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs = Float.POSITIVE_INFINITY.toInt()
  override val numberOfOutputs = 0

  private fun setVolumeAndPanning(playbackParameters: PlaybackParameters) {
    playbackParameters.audioTrack.setStereoVolume(playbackParameters.leftPan.toFloat(), playbackParameters.rightPan.toFloat())
  }

  override fun process(playbackParameters: PlaybackParameters) {
    setVolumeAndPanning(playbackParameters)
    playbackParameters.audioTrack.write(playbackParameters.buffer, 0, playbackParameters.buffer.size)
  }
}

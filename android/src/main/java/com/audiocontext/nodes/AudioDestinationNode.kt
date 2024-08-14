package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.PlaybackParameters


class AudioDestinationNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs = Float.POSITIVE_INFINITY.toInt()
  override val numberOfOutputs = 0

  private fun setVolumeAndPanning(playbackParameters: PlaybackParameters) {
    val leftPan = playbackParameters.gain * playbackParameters.leftPan
    val rightPan = playbackParameters.gain * playbackParameters.rightPan
    playbackParameters.audioTrack.setStereoVolume(leftPan.toFloat(), rightPan.toFloat())
  }

  override fun process(playbackParameters: PlaybackParameters) {
    setVolumeAndPanning(playbackParameters)
    playbackParameters.audioTrack.write(playbackParameters.buffer, 0, playbackParameters.buffer.size)
  }
}

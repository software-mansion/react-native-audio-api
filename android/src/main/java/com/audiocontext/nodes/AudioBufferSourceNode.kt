package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.AudioBuffer

class AudioBufferSourceNode(context: BaseAudioContext) : AudioScheduledSourceNode(context) {
  override var playbackParameters: PlaybackParameters? = null

  private var loop: Boolean = false
    get() = field
  private var buffer: AudioBuffer? = null
    get() = field

  fun setBuffer(buffer: AudioBuffer) {
    val audioTrack = context.getAudioTrack()
    this.buffer = buffer.copy()
    playbackParameters = PlaybackParameters(audioTrack, buffer)
  }

  fun setLoop(value: Boolean) {
    loop = value
  }

  override fun fillBuffer(playbackParameters: PlaybackParameters) {
    if (!loop) {
      isPlaying = false
    } else {
      playbackParameters.audioBuffer = buffer!!.copy()
    }
  }
}

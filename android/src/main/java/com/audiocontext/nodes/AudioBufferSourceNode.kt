package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.AudioBuffer
import com.audiocontext.utils.Constants

class AudioBufferSourceNode(context: BaseAudioContext) : AudioScheduledSourceNode(context) {
  override var playbackParameters: PlaybackParameters? = null

  private var loop: Boolean = false
    get() = field
  private var buffer: AudioBuffer = AudioBuffer(context.sampleRate, Constants.BUFFER_SIZE, 2)
    get() = field

  init {
    playbackParameters = PlaybackParameters(context.getAudioTrack(Constants.BUFFER_SIZE), buffer)
  }

  fun setBuffer(buffer: AudioBuffer) {
    val audioTrack = context.getAudioTrack(2 * buffer.length)
    this.buffer = buffer.copy()
    playbackParameters = PlaybackParameters(audioTrack, buffer)
    channelCount = buffer.numberOfChannels
  }

  fun setLoop(value: Boolean) {
    loop = value
  }

  override fun fillBuffer(playbackParameters: PlaybackParameters) {
    if (!loop) {
      isPlaying = false
    } else {
      playbackParameters.audioBuffer = buffer.copy()
    }
  }
}

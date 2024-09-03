package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.AudioBuffer
import com.audiocontext.utils.Constants

class AudioBufferSourceNode(context: BaseAudioContext) : AudioScheduledSourceNode(context) {
  override var playbackParameters: PlaybackParameters? = null

  private var loop: Boolean = true
    get() = field
  private var buffer: AudioBuffer
    get() = field

  init {
    buffer = AudioBuffer(context.sampleRate, Constants.BUFFER_SIZE, 2)
    playbackParameters = PlaybackParameters(context.getAudioTrack(Constants.BUFFER_SIZE), buffer)
  }

  fun setBuffer(audioBuffer: AudioBuffer) {
    playbackParameters?.let { context.addAudioTrack(it.audioTrack) }
    this.buffer = audioBuffer
    channelCount = audioBuffer.numberOfChannels

    val audioTrack = context.getAudioTrack(2 * buffer.length)
    val buffer = AudioBuffer(buffer.sampleRate, buffer.length, buffer.numberOfChannels)
    playbackParameters = PlaybackParameters(audioTrack, buffer)
  }

  fun setLoop(value: Boolean) {
    loop = value
  }

  override fun fillBuffer(playbackParameters: PlaybackParameters) {
    mixBuffers(playbackParameters)

    if (!loop) {
      isPlaying = false
    } else {
      for (i in 0 until playbackParameters.audioBuffer.numberOfChannels) {
        for (j in 0 until playbackParameters.audioBuffer.length) {
          playbackParameters.audioBuffer.getChannelData(i)[j] = buffer.getChannelData(i)[j % buffer.length]
        }
      }
    }
  }
}

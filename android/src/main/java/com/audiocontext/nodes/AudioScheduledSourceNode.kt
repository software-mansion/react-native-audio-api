package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.audionode.AudioNode
import com.audiocontext.parameters.PlaybackParameters

abstract class AudioScheduledSourceNode(context: BaseAudioContext) : AudioNode(context) {
  override val numberOfInputs: Int = 0
  override val numberOfOutputs: Int = 1

  abstract var playbackParameters: PlaybackParameters?
  @Volatile protected var isPlaying: Boolean = false

  private var playbackThread: Thread? = null
  private var stopTime: Double? = null

  private fun generateSound() {
    while(isPlaying) {
      playbackParameters?.let { fillBuffer(it) }
      playbackParameters?.reset()
      playbackParameters?.let { process(it) }
      if(stopTime != null && context.getCurrentTime() >= stopTime!!) {
        prepareForDeconstruction()
      }
    }
  }

  private fun handleStop(){
    isPlaying = false
    try {
      playbackParameters?.audioTrack?.stop()
      playbackParameters?.audioTrack?.flush()
    } catch (e: IllegalStateException) {
      e.printStackTrace()
    }

    playbackThread?.interrupt()
  }

  protected abstract fun fillBuffer(playbackParameters: PlaybackParameters)

  fun start(time: Double) {
    if (playbackParameters == null) {
      throw IllegalStateException("You can use source only once")
    }

    playbackThread = Thread {
      while (context.getCurrentTime() < time) {
        Thread.sleep(1)
      }
      isPlaying = true
      playbackParameters?.audioTrack?.play()
      generateSound()
    }
    playbackThread?.start()
  }

  fun stop(time: Double) {
    stopTime = time
  }

  private fun prepareForDeconstruction() {
    handleStop()
    playbackParameters?.let { context.addAudioTrack(it.audioTrack) }
    playbackParameters = null
  }

  override fun cleanup() {
    prepareForDeconstruction()
    super.cleanup()
  }
}

package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.PlaybackParameters
import java.util.PriorityQueue

abstract class AudioScheduledSourceNode(context: BaseAudioContext) : AudioNode(context) {
  override val numberOfInputs: Int = 0
  override val numberOfOutputs: Int = 1

  protected var playbackParameters: PlaybackParameters = context.getPlaybackParameters()
  @Volatile protected var isPlaying: Boolean = false

  private var playbackThread: Thread? = null;
  private val stopQueue = PriorityQueue<Double>()

  private fun generateSound() {
    while(isPlaying) {
      generateBuffer()
      playbackParameters.reset()
      process(playbackParameters)
      if(stopQueue.isNotEmpty() && context.getCurrentTime() >= stopQueue.peek()!!) {
        handleStop()
        stopQueue.poll()
      }
    }
    playbackParameters.audioTrack.flush()
  }

  private fun handleStop(){
    isPlaying = false
    try {
      playbackParameters.audioTrack.stop()
    } catch (e: IllegalStateException) {
      e.printStackTrace()
    }

    playbackThread?.interrupt()
  }

  protected abstract fun generateBuffer()

  fun start(time: Double) {
    playbackThread = Thread {
      while (context.getCurrentTime() < time) {
        Thread.sleep(1)
      }
      isPlaying = true
      playbackParameters.audioTrack.play()
      generateSound()
    }
    playbackThread?.start()
  }

  fun stop(time: Double) {
    stopQueue.add(time)
  }

  private fun prepareForDeconstruction() {
    handleStop()
    context.addAudioTrack(playbackParameters.audioTrack)
  }

  override fun close() {
    try {
      handleStop()
    } catch (e: IllegalStateException) {
      e.printStackTrace()
    }

    super.close()
  }
}

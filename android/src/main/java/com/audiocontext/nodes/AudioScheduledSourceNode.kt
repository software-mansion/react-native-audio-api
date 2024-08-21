package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.PlaybackParameters
import java.util.PriorityQueue

abstract class AudioScheduledSourceNode(context: BaseAudioContext) : AudioNode(context) {
  override val numberOfInputs: Int = 0
  override val numberOfOutputs: Int = 1

  private var playbackParameters: PlaybackParameters? = context.getPlaybackParameters()
  @Volatile protected var isPlaying: Boolean = false

  private var playbackThread: Thread? = null;
  private val stopQueue = PriorityQueue<Double>()

  private fun generateSound() {
    while(isPlaying) {
      playbackParameters?.let { generateBuffer(it) }
      playbackParameters?.reset()
      playbackParameters?.let { process(it) }
      if(stopQueue.isNotEmpty() && context.getCurrentTime() >= stopQueue.peek()!!) {
        prepareForDeconstruction()
        stopQueue.poll()
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

  protected abstract fun generateBuffer(playbackParameters: PlaybackParameters)

  fun start(time: Double) {
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
    stopQueue.add(time)
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

package com.audiocontext.nodes

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.PlaybackParameters
import java.util.PriorityQueue

abstract class AudioScheduledSourceNode(context: BaseAudioContext) : AudioNode(context) {
  override val numberOfInputs: Int = 0
  override val numberOfOutputs: Int = 1

  protected var playbackParameters: PlaybackParameters
  @Volatile protected var isPlaying: Boolean = false

  private var playbackThread: Thread? = null;
  private val stopQueue = PriorityQueue<Double>()

  init {
    val bufferSize = AudioTrack.getMinBufferSize(
      context.sampleRate,
      AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT)/10

    val audioAttributes = AudioAttributes.Builder()
      .setUsage(AudioAttributes.USAGE_MEDIA)
      .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
      .build()

    val audioFormat = AudioFormat.Builder()
      .setSampleRate(context.sampleRate)
      .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
      .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
      .build()

    val audioTrack = AudioTrack(audioAttributes, audioFormat, bufferSize, AudioTrack.MODE_STREAM, AudioManager.AUDIO_SESSION_ID_GENERATE)
    val buffer = ShortArray(bufferSize)

    this.playbackParameters = PlaybackParameters(audioTrack, buffer)
  }

  private fun generateSound() {
    while(isPlaying) {
      generateBuffer()
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
    playbackParameters.audioTrack.stop()
    playbackThread?.join()
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

  fun close() {
    handleStop()
    playbackParameters.audioTrack.release()
  }
}

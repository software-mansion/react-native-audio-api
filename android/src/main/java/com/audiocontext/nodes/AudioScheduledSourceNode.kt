package com.audiocontext.nodes

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.PlaybackParameters
import kotlinx.coroutines.*
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock

abstract class AudioScheduledSourceNode(context: BaseAudioContext) : AudioNode(context) {
  override val numberOfInputs: Int = 0
  override val numberOfOutputs: Int = 1

  protected var playbackParameters: PlaybackParameters
  @Volatile protected var isPlaying: Boolean = false

  private var playbackThread: Thread? = null;

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

  protected abstract fun generateSound();

  fun start(time: Double) {
//    coroutineScope.launch {
//      mutex.withLock {
//        if (audioJob?.isActive == true) return@launch
//        audioJob = launch {
//          while (context.getCurrentTime() < time) {
//            delay(1)
//          }
//          isPlaying = true
//          playbackParameters.audioTrack.play()
//          generateSound()
//        }
//      }
//    }
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
    Thread {
      while (context.getCurrentTime() < time) {
        Thread.sleep(1)
      }
      isPlaying = false
      playbackParameters.audioTrack.stop()
      playbackThread?.interrupt()
    }.start()
  }
}

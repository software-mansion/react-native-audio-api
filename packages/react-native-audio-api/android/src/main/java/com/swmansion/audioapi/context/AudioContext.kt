package com.swmansion.audioapi.context

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.os.Build
import android.os.SystemClock
import androidx.annotation.RequiresApi
import com.facebook.jni.HybridData
import com.swmansion.audioapi.nodes.AudioBufferSourceNode
import com.swmansion.audioapi.nodes.AudioScheduledSourceNode
import com.swmansion.audioapi.nodes.GainNode
import com.swmansion.audioapi.nodes.StereoPannerNode
import com.swmansion.audioapi.nodes.filter.BiquadFilterNode
import com.swmansion.audioapi.nodes.oscillator.OscillatorNode
import com.swmansion.audioapi.utils.AudioBuffer
import com.swmansion.audioapi.utils.Constants
import java.util.LinkedList

class AudioContext : BaseAudioContext {
  override val sampleRate: Int = Constants.SAMPLE_RATE
    get() = field

  // override val destination: AudioDestinationNode = AudioDestinationNode(this)
  //  get() = field
  override var state = ContextState.RUNNING

  private val contextStartTime: Long
  private val mHybridData: HybridData?
  private val sources: MutableList<AudioScheduledSourceNode> = mutableListOf()
  private val audioTracksList = LinkedList<AudioTrack>()

  companion object {
    init {
      System.loadLibrary("react-native-audio-api")
    }
  }

  init {
    mHybridData = initHybrid()
    contextStartTime = SystemClock.elapsedRealtimeNanos()
  }

  external fun initHybrid(): HybridData?

  private fun initAudioTrack(bufferSize: Int): AudioTrack {
    val audioAttributes =
      AudioAttributes
        .Builder()
        .setUsage(AudioAttributes.USAGE_MEDIA)
        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
        .build()

    val audioFormat =
      AudioFormat
        .Builder()
        .setSampleRate(this.sampleRate)
        .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
        .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO)
        .build()

    val audioTrack = AudioTrack(audioAttributes, audioFormat, bufferSize, AudioTrack.MODE_STREAM, AudioManager.AUDIO_SESSION_ID_GENERATE)

    if (audioTrack.state != AudioTrack.STATE_INITIALIZED) {
      throw IllegalStateException("Failed to initialize AudioTrack")
    }

    return audioTrack
  }

  override fun getCurrentTime(): Double = (SystemClock.elapsedRealtimeNanos() - contextStartTime) / 1e9

  fun getState(): String = ContextState.toString(state)

  override fun close() {
    state = ContextState.CLOSED
    sources.forEach { it.cleanup() }
    sources.clear()
    audioTracksList.forEach { it.release() }
    audioTracksList.clear()
  }

  @RequiresApi(Build.VERSION_CODES.N)
  override fun getAudioTrack(bufferSize: Int): AudioTrack {
    synchronized(audioTracksList) {
      if (audioTracksList.isNotEmpty()) {
        return audioTracksList.removeFirst()
      } else {
        val audioTrack = initAudioTrack(bufferSize)

        return audioTrack
      }
    }
  }

  override fun addAudioTrack(audioTrack: AudioTrack) {
    synchronized(audioTracksList) {
      audioTracksList.add(audioTrack)
    }
  }

  override fun createOscillator(): OscillatorNode {
    val oscillatorNode = OscillatorNode(this)
    sources.add(oscillatorNode)
    return oscillatorNode
  }

  override fun createGain(): GainNode = GainNode(this)

  override fun createStereoPanner(): StereoPannerNode = StereoPannerNode(this)

  override fun createBiquadFilter(): BiquadFilterNode = BiquadFilterNode(this)

  override fun createBufferSource(): AudioBufferSourceNode {
    val bufferSourceNode = AudioBufferSourceNode(this)
    sources.add(bufferSourceNode)
    return bufferSourceNode
  }

  override fun createBuffer(
    numOfChannels: Int,
    length: Int,
    sampleRate: Int,
  ): AudioBuffer {
    val buffer = AudioBuffer(numOfChannels, length, sampleRate)
    return buffer
  }
}

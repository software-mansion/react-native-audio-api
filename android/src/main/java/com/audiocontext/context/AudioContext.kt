package com.audiocontext.context

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.os.SystemClock
import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.AudioScheduledSourceNode
import com.audiocontext.utils.Constants
import com.audiocontext.nodes.GainNode
import com.audiocontext.nodes.StereoPannerNode
import com.audiocontext.nodes.oscillator.OscillatorNode
import com.audiocontext.parameters.PlaybackParameters
import com.facebook.jni.HybridData
import java.util.LinkedList

class AudioContext() : BaseAudioContext {
  override val sampleRate: Int = Constants.SAMPLE_RATE
    get() = field
  override val destination: AudioDestinationNode = AudioDestinationNode(this)
    get() = field
  override var state = ContextState.RUNNING

  private val contextStartTime: Long
  private val mHybridData: HybridData?
  private val sources: MutableList<AudioScheduledSourceNode> = mutableListOf()
  private val audioTracksList = LinkedList<AudioTrack>()

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid()
    contextStartTime = SystemClock.elapsedRealtimeNanos()
  }

  external fun initHybrid(): HybridData?
  external fun install(jsContext: Long)

  private fun initAudioTrack(bufferSize: Int): AudioTrack {
    val audioAttributes = AudioAttributes.Builder()
      .setUsage(AudioAttributes.USAGE_MEDIA)
      .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
      .build()

    val audioFormat = AudioFormat.Builder()
      .setSampleRate(this.sampleRate)
      .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
      .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
      .build()

    return AudioTrack(audioAttributes, audioFormat, bufferSize, AudioTrack.MODE_STREAM, AudioManager.AUDIO_SESSION_ID_GENERATE)
  }

  override fun getCurrentTime(): Double {
    return (SystemClock.elapsedRealtimeNanos() - contextStartTime) / 1e9
  }

  fun getState(): String {
    return ContextState.toString(state)
  }

  override fun close() {
    state = ContextState.CLOSED
    sources.forEach { it.close() }
    audioTracksList.forEach { it.release() }
    audioTracksList.clear()
    sources.clear()
  }

  override fun getPlaybackParameters(): PlaybackParameters {
    val bufferSize = 128

    val buffer = ShortArray(bufferSize)

    if(audioTracksList.isNotEmpty()) {
      return PlaybackParameters(audioTracksList.removeFirst(), buffer)
    } else {
      val audioTrack = initAudioTrack(bufferSize)

      return PlaybackParameters(audioTrack, buffer)
    }
  }

  override fun addAudioTrack(audioTrack: AudioTrack) {
    audioTracksList.add(audioTrack)
  }

  override fun createOscillator(): OscillatorNode {
    val oscillatorNode = OscillatorNode(this)
    sources.add(oscillatorNode)
    return oscillatorNode
  }

  override fun createGain(): GainNode {
    return GainNode(this)
  }

  override fun createStereoPanner(): StereoPannerNode {
    return StereoPannerNode(this)
  }
}

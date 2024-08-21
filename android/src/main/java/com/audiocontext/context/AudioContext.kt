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
import com.audiocontext.nodes.filter.BiquadFilterNode
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

  private fun initAudioTrack(): AudioTrack {
    val audioAttributes = AudioAttributes.Builder()
      .setUsage(AudioAttributes.USAGE_MEDIA)
      .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
      .build()

    val audioFormat = AudioFormat.Builder()
      .setSampleRate(this.sampleRate)
      .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
      .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
      .build()

    val audioTrack = AudioTrack(audioAttributes, audioFormat, Constants.BUFFER_SIZE, AudioTrack.MODE_STREAM, AudioManager.AUDIO_SESSION_ID_GENERATE)

    if (audioTrack.state != AudioTrack.STATE_INITIALIZED) {
      throw IllegalStateException("Failed to initialize AudioTrack")
    }

    return audioTrack
  }

  override fun getCurrentTime(): Double {
    return (SystemClock.elapsedRealtimeNanos() - contextStartTime) / 1e9
  }

  fun getState(): String {
    return ContextState.toString(state)
  }

  override fun close() {
    state = ContextState.CLOSED
    audioTracksList.forEach { it.release() }
    audioTracksList.clear()
    sources.forEach { it.close() }
    sources.clear()
  }

  override fun getPlaybackParameters(): PlaybackParameters {
    val buffer = ShortArray(Constants.BUFFER_SIZE)

    synchronized(audioTracksList) {
      if(audioTracksList.isNotEmpty()) {
        return PlaybackParameters(audioTracksList.removeFirst(), buffer)
      } else {
        val audioTrack = initAudioTrack()

        return PlaybackParameters(audioTrack, buffer)
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

  override fun createGain(): GainNode {
    return GainNode(this)
  }

  override fun createStereoPanner(): StereoPannerNode {
    return StereoPannerNode(this)
  }

  override fun createBiquadFilter(): BiquadFilterNode {
    return BiquadFilterNode(this)
  }
}

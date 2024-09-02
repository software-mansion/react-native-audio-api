package com.audiocontext.context

import android.media.AudioTrack
import com.audiocontext.nodes.AudioBufferSourceNode
import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.GainNode
import com.audiocontext.nodes.StereoPannerNode
import com.audiocontext.nodes.filter.BiquadFilterNode
import com.audiocontext.nodes.oscillator.OscillatorNode
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.AudioBuffer

interface BaseAudioContext {
  val sampleRate: Int
  val destination: AudioDestinationNode
  var state: ContextState

  abstract fun getCurrentTime(): Double
  abstract fun getAudioTrack(bufferSize: Int): AudioTrack
  abstract fun addAudioTrack(audioTrack: AudioTrack)
  abstract fun close()
  abstract fun createOscillator(): OscillatorNode
  abstract fun createGain(): GainNode
  abstract fun createStereoPanner(): StereoPannerNode
  abstract fun createBiquadFilter(): BiquadFilterNode
  abstract fun createBufferSource(): AudioBufferSourceNode
  abstract fun createBuffer(numOfChannels: Int, length: Int, sampleRate: Int): AudioBuffer
}

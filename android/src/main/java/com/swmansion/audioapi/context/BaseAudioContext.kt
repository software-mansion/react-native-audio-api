package com.swmansion.audioapi.context

import android.media.AudioTrack
import com.swmansion.audioapi.nodes.AudioBufferSourceNode
import com.swmansion.audioapi.nodes.AudioDestinationNode
import com.swmansion.audioapi.nodes.GainNode
import com.swmansion.audioapi.nodes.StereoPannerNode
import com.swmansion.audioapi.nodes.filter.BiquadFilterNode
import com.swmansion.audioapi.nodes.oscillator.OscillatorNode
import com.swmansion.audioapi.utils.AudioBuffer

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

  abstract fun createBuffer(
    numOfChannels: Int,
    length: Int,
    sampleRate: Int,
  ): AudioBuffer
}

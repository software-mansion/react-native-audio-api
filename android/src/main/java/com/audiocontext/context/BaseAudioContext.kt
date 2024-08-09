package com.audiocontext.context

import android.media.AudioTrack
import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.GainNode
import com.audiocontext.nodes.StereoPannerNode
import com.audiocontext.nodes.oscillator.OscillatorNode
import com.audiocontext.parameters.PlaybackParameters

interface BaseAudioContext {
  val sampleRate: Int
  val destination: AudioDestinationNode
  var state: ContextState

  abstract fun getCurrentTime(): Double
  abstract fun getPlaybackParameters(): PlaybackParameters
  abstract fun addAudioTrack(audioTrack: AudioTrack)
  abstract fun close()
  abstract fun createOscillator(): OscillatorNode
  abstract fun createGain(): GainNode
  abstract fun createStereoPanner(): StereoPannerNode
}

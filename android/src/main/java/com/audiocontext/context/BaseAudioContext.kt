package com.audiocontext.context

import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.GainNode
import com.audiocontext.nodes.StereoPannerNode
import com.audiocontext.nodes.oscillator.OscillatorNode

interface BaseAudioContext {
  val sampleRate: Int
  val destination: AudioDestinationNode

  abstract fun createOscillator(): OscillatorNode
  abstract fun createGain(): GainNode
  abstract fun createStereoPanner(): StereoPannerNode
}

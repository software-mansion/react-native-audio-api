package com.audiocontext.context

import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.oscillator.OscillatorNode

interface BaseAudioContext {
  val sampleRate: Int
  val destination: AudioDestinationNode

  abstract fun createOscillator(): OscillatorNode
}

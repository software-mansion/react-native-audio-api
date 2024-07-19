package com.audiocontext.context

import android.media.AudioTrack
import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.AudioNode
import com.audiocontext.nodes.oscillator.OscillatorNode
import com.facebook.jni.HybridData

interface BaseAudioContext {
  val sampleRate: Int
  val destination: AudioDestinationNode

  abstract fun createOscillator(): OscillatorNode
}

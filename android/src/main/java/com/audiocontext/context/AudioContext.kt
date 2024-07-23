package com.audiocontext.context

import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.oscillator.OscillatorNode
import com.facebook.jni.HybridData

class AudioContext() : BaseAudioContext {
  override val sampleRate: Int = 44100
  override val destination: AudioDestinationNode = AudioDestinationNode(this)
    get() = field

  private val mHybridData: HybridData?;

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid()
  }

  external fun initHybrid(): HybridData?
  external fun install(jsContext: Long)

  override fun createOscillator(): OscillatorNode {
    return OscillatorNode(this)
  }
}

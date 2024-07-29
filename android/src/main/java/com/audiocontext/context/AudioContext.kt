package com.audiocontext.context

import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.GainNode
import com.audiocontext.nodes.StereoPannerNode
import com.audiocontext.nodes.oscillator.OscillatorNode
import com.facebook.jni.HybridData

class AudioContext() : BaseAudioContext {
  override val sampleRate: Int = 44100
    get() = field
  override val destination: AudioDestinationNode = AudioDestinationNode(this)
    get() = field
  override var state = ContextState.RUNNING

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

  fun getState(): String {
    return ContextState.toString(state)
  }

  override fun createOscillator(): OscillatorNode {
    return OscillatorNode(this)
  }

  override fun createGain(): GainNode {
    return GainNode(this)
  }

  override fun createStereoPanner(): StereoPannerNode {
    return StereoPannerNode(this)
  }
}

package com.audiocontext.context

import android.os.SystemClock
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
  private val contextStartTime: Long

  private val mHybridData: HybridData?;

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

  override fun getCurrentTime(): Double {
    return (SystemClock.elapsedRealtimeNanos() - contextStartTime) / 1e9
  }

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

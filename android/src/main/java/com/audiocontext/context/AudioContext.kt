package com.audiocontext.context

import android.media.AudioTrack
import com.audiocontext.nodes.AudioDestinationNode
import com.audiocontext.nodes.AudioNode
import com.audiocontext.nodes.oscillator.OscillatorNode
import com.facebook.jni.HybridData
import com.facebook.react.bridge.ReactApplicationContext
import java.util.concurrent.CopyOnWriteArrayList

class AudioContext(private val reactContext: ReactApplicationContext) : BaseAudioContext {
  override val sampleRate: Int = 44100
  override val destination: AudioDestinationNode = AudioDestinationNode(this, reactContext)
    get() = field

  private val mHybridData: HybridData?;

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid(reactContext.javaScriptContextHolder!!.get())
  }

  external fun initHybrid(l: Long): HybridData?

  override fun createOscillator(): OscillatorNode {
    return OscillatorNode(this, reactContext)
  }
}

package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.PlaybackParameters
import com.facebook.jni.HybridData


abstract class AudioNode(val context: BaseAudioContext) {
  open val numberOfInputs: Int = 0
    get() = field
  open val numberOfOutputs: Int = 0
    get() = field
  protected val connectedNodes = mutableListOf<AudioNode>()

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

  fun connect(node: AudioNode) {
    if(this.numberOfOutputs > connectedNodes.size) {
      connectedNodes.add(node)
    }
  }

  fun disconnect() {
    connectedNodes.clear()
  }

  open fun process(playbackParameters: PlaybackParameters) {
    connectedNodes.forEach { it.process(playbackParameters) }
  }

  open fun close() {
    connectedNodes.forEach { it.close() }
    connectedNodes.clear()
  }
}

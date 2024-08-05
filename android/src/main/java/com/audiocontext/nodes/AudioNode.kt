package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.PlaybackParameters
import com.facebook.jni.HybridData


abstract class AudioNode(val context: BaseAudioContext) {
  open val numberOfInputs: Int = 0
    get() = field
  open val numberOfOutputs: Int = 0
    get() = field
  private val connectedNodes = mutableListOf<AudioNode>()

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
    if(this.numberOfOutputs > 0) {
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
    mHybridData?.resetNative()
    connectedNodes.forEach { it.close() }
    connectedNodes.clear()
  }
}

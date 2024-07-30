package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.PlaybackParameters
import com.facebook.jni.HybridData


abstract class AudioNode(val context: BaseAudioContext) {
  abstract val numberOfInputs: Int;
  abstract val numberOfOutputs: Int;
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
}

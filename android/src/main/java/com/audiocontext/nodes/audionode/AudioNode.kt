package com.audiocontext.nodes.audionode

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.PlaybackParameters
import com.facebook.jni.HybridData


abstract class AudioNode(val context: BaseAudioContext) {
  open val numberOfInputs: Int = 0
    get() = field
  open val numberOfOutputs: Int = 0
    get() = field
  open val channelCount: Int = 1
    get() = field
  open val channelCountMode: ChannelCountMode = ChannelCountMode.MAX
    get() = field
  open val channelInterpretation: Channelnterpretation = Channelnterpretation.SPEAKERS
    get() = field
  private val inputNodes = mutableListOf<AudioNode>()
  private val outputNodes = mutableListOf<AudioNode>()

  private val mHybridData: HybridData?

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
    if(numberOfOutputs > outputNodes.size && node.numberOfInputs > node.inputNodes.size) {
      outputNodes.add(node)
      node.inputNodes.add(this)
    }
  }

  fun disconnect(node: AudioNode) {
    outputNodes.remove(node)
    node.inputNodes.remove(this)
  }

  open fun process(playbackParameters: PlaybackParameters) {
    outputNodes.forEach { it.process(playbackParameters) }
  }

  open fun cleanup() {
    outputNodes.forEach { it.cleanup() }
    inputNodes.clear()
    outputNodes.clear()
  }

  protected fun mixBuffers(playbackParameters: PlaybackParameters) {
    val buffer = playbackParameters.audioBuffer.mix(channelCount)
    playbackParameters.audioBuffer = buffer
  }
}

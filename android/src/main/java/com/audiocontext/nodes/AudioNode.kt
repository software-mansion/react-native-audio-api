package com.audiocontext.nodes

import android.media.AudioTrack
import com.audiocontext.context.BaseAudioContext


abstract class AudioNode(val context: BaseAudioContext) {
  abstract val numberOfInputs: Int;
  abstract val numberOfOutputs: Int;
  private val connectedNodes = mutableListOf<AudioNode>()

  fun connect(destination: AudioDestinationNode) {
    if(this.numberOfOutputs > 0) {
      connectedNodes.add(destination)
    }
  }

  fun disconnect() {
    connectedNodes.clear()
  }

  open fun process(buffer: ShortArray, audioTrack: AudioTrack) {
    connectedNodes.forEach { it.process(buffer, audioTrack) }
  }
}

package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.AudioParam
import com.audiocontext.nodes.parameters.PlaybackParameters
import com.facebook.jni.HybridData

class GainNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  private var gain: AudioParam = AudioParam(1.0, 1.0, 0.0)
    get() = field
    set(value) {
      field = value
    }

  private val mHybridData: HybridData? = initHybrid();

  override fun process(playbackParameters: PlaybackParameters) {
    playbackParameters.gain = gain.getValue()
    super.process(playbackParameters)
  }
}

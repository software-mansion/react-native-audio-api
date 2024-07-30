package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.AudioParam
import com.audiocontext.nodes.parameters.PlaybackParameters
import com.facebook.jni.HybridData
import kotlin.math.min

class StereoPannerNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  private var pan: AudioParam = AudioParam(0.0, 1.0, -1.0)
    get() = field
    set(value) {
      field = value
    }

  private val mHybridData: HybridData? = initHybrid();

  override fun process(playbackParameters: PlaybackParameters) {
    playbackParameters.leftPan = min(1.0 - pan.getValue(), 1.0)
    playbackParameters.rightPan = min(1.0 + pan.getValue(), 1.0)
    super.process(playbackParameters)
  }
}

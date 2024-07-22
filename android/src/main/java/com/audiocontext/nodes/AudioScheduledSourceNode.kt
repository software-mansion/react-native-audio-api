package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext

abstract class AudioScheduledSourceNode(context: BaseAudioContext) : AudioNode(context) {
  abstract fun start(time: Double)
  abstract fun stop(time: Double)
}

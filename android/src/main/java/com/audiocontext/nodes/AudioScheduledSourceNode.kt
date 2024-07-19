package com.audiocontext.nodes

import com.audiocontext.context.BaseAudioContext

abstract class AudioScheduledSourceNode(context: BaseAudioContext) : AudioNode(context) {
  abstract fun start()
  abstract fun stop()
}

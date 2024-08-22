package com.audiocontext.nodes

import android.os.Build
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.AudioParam
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.Constants
import kotlin.math.min

class StereoPannerNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  private val pan: AudioParam = AudioParam(context,0.0, Constants.MAX_PAN, -Constants.MAX_PAN)
    get() = field

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    double currentTime = context.getCurrentTime()
    playbackParameters.leftPan *= min(1.0 - pan.getValueAtTime(currentTime), 1.0)
    playbackParameters.rightPan *= min(1.0 + pan.getValueAtTime(currentTime), 1.0)
    super.process(playbackParameters)
  }
}

package com.audiocontext.nodes

import android.os.Build
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.audionode.AudioNode
import com.audiocontext.nodes.audionode.ChannelCountMode
import com.audiocontext.parameters.AudioParam
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.Constants
import kotlin.math.*

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

class StereoPannerNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  override var channelCount: Int = 2
  override val channelCountMode: ChannelCountMode = ChannelCountMode.CLAMPED_MAX

  private val pan: AudioParam = AudioParam(context,0.0, Constants.MAX_PAN, -Constants.MAX_PAN)
    get() = field

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    mixBuffers(playbackParameters)

    val pan = pan.getValueAtTime(context.getCurrentTime())
    val x = (pan + 1) / 2

    val gainL = cos(x * PI / 2)
    val gainR = sin(x * PI / 2)

    playbackParameters.leftPan *= gainL
    playbackParameters.rightPan *= gainR

    super.process(playbackParameters)
  }
}

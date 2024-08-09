package com.audiocontext.nodes.filter

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.AudioNode
import com.audiocontext.parameters.AudioParam
import com.audiocontext.utils.Constants

class BiquadFilterNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  private val frequency: AudioParam = AudioParam(context, 350.0, Constants.MAX_FILTER_FREQUENCY, Constants.MIN_FILTER_FREQUENCY)
    get() = field
  private val detune: AudioParam = AudioParam(context, 0.0, Constants.MAX_DETUNE, -Constants.MAX_DETUNE)
    get() = field
  private val Q: AudioParam = AudioParam(context, 1.0, Constants.MAX_FILTER_Q, -Constants.MAX_FILTER_Q)
    get() = field
  private val gain: AudioParam = AudioParam(context, 0.0, Constants.MAX_FILTER_GAIN, Constants.MIN_FILTER_GAIN)
  private var filterType: FilterType = FilterType.LOWPASS

  fun getFilterType(): String {
    return FilterType.toString(filterType)
  }

  fun setFilterType(type: String) {
    filterType = FilterType.fromString(type)
  }

}

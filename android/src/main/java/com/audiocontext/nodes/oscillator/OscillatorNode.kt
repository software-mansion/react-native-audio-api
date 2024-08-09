package com.audiocontext.nodes.oscillator

import android.os.Build
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.parameters.AudioParam
import com.audiocontext.nodes.AudioScheduledSourceNode
import com.audiocontext.utils.Constants
import kotlin.math.pow

class OscillatorNode(context: BaseAudioContext) : AudioScheduledSourceNode(context) {
  private val frequency: AudioParam = AudioParam(context, 440.0, Constants.NYQUIST_FREQUENCY, -Constants.NYQUIST_FREQUENCY)
    get() = field
  private val detune: AudioParam = AudioParam(context, 0.0, Constants.MAX_DETUNE, -Constants.MAX_DETUNE)
    get() = field
  private var waveType: WaveType = WaveType.SINE
  private var wavePhase: Double = 0.0

  fun getWaveType(): String {
    return WaveType.toString(waveType)
  }

  fun setWaveType(type: String) {
    waveType = WaveType.fromString(type)
  }

  @RequiresApi(Build.VERSION_CODES.N)
  override fun generateBuffer() {
    for(i in playbackParameters.buffer.indices) {
      val computedFrequency = frequency.getValueAtTime(context.getCurrentTime()) * 2.0.pow(detune.getValueAtTime(context.getCurrentTime())/ 1200.0)
      wavePhase += 2.0 * Math.PI * (computedFrequency / context.sampleRate)

      if (wavePhase > 2.0 * Math.PI) {
        wavePhase -= 2.0 * Math.PI
      }
      playbackParameters.buffer[i] = WaveType.getWaveBufferElement(wavePhase, waveType)
    }
  }
}

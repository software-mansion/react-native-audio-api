package com.audiocontext.nodes.oscillator

import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.AudioParam
import com.audiocontext.nodes.AudioScheduledSourceNode

class OscillatorNode(context: BaseAudioContext) : AudioScheduledSourceNode(context) {
  private var frequency: AudioParam = AudioParam(440.0, 1200.0, 100.0)
    get() = field
    set(value) {
      field = value
    }
  private var detune: AudioParam = AudioParam(0.0, 100.0, -100.0)
    get() = field
    set(value) {
      field = value
    }
  private var waveType: WaveType = WaveType.SINE

  fun getWaveType(): String {
    return WaveType.toString(waveType)
  }

  fun setWaveType(type: String) {
    waveType = WaveType.fromString(type)
  }

  override fun generateSound() {
    var wavePhase = 0.0
    var phaseChange: Double

    while(isPlaying) {
      phaseChange = 2 * Math.PI * (frequency.getValue() + detune.getValue()) / context.sampleRate

      for(i in playbackParameters.buffer.indices) {
        playbackParameters.buffer[i] = WaveType.getWaveBufferElement(wavePhase, waveType)
        wavePhase += phaseChange
      }
      process(playbackParameters)
    }
    playbackParameters.audioTrack.flush()
  }
}

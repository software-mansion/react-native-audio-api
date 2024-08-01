package com.audiocontext.nodes.oscillator

import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.AudioParam
import com.audiocontext.nodes.AudioScheduledSourceNode
import kotlin.math.pow

class OscillatorNode(context: BaseAudioContext) : AudioScheduledSourceNode(context) {
  private var frequency: AudioParam = AudioParam(context, 440.0, context.sampleRate/2.0, 0.0)
    get() = field
    set(value) {
      field = value
    }
  private var detune: AudioParam = AudioParam(context, 0.0, 100.0, -100.0)
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

  @RequiresApi(Build.VERSION_CODES.N)
  override fun generateSound() {
    var wavePhase = 0.0

    while(isPlaying) {
      for(i in playbackParameters.buffer.indices) {
        val computedFrequency = frequency.getValueAtTime(context.getCurrentTime()) * 2.0.pow(detune.getValueAtTime(context.getCurrentTime())/ 1200.0)
        wavePhase += 2.0 * Math.PI * (computedFrequency / context.sampleRate)

        if (wavePhase > 2.0 * Math.PI) {
          wavePhase -= 2.0 * Math.PI
        }
        playbackParameters.buffer[i] = WaveType.getWaveBufferElement(wavePhase, waveType)

      }
      process(playbackParameters)
    }
    playbackParameters.audioTrack.flush()
  }

  fun finalize(){
    playbackParameters.audioTrack.stop()
    playbackParameters.audioTrack.release()
    Log.d("OscillatorNode", "OscillatorNode finalized")
  }
}

package com.audiocontext.nodes.oscillator

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.util.Log
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.AudioScheduledSourceNode
import com.facebook.jni.HybridData
import com.facebook.react.bridge.ReactApplicationContext
import kotlin.math.abs
import kotlin.math.floor
import kotlin.math.sin

class OscillatorNode(context: BaseAudioContext) : AudioScheduledSourceNode(context) {
  override val numberOfInputs: Int = 0
  override val numberOfOutputs: Int = 1
  private var frequency: Double = 440.0
    get() = field
    set(value) {
      field = value
    }
  private var detune: Double = 0.0
    get() = field
    set(value) {
      field = value
    }
  private var waveType: WaveType = WaveType.SINE

  private val audioTrack: AudioTrack
  @Volatile private var isPlaying: Boolean = false
  private var playbackThread: Thread? = null
  private var stopThread: Thread? = null
  private var buffer: ShortArray = ShortArray(1024)

  private val mHybridData: HybridData?;

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid()
    val bufferSize = AudioTrack.getMinBufferSize(
      context.sampleRate,
      AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT)
    this.audioTrack = AudioTrack(
      AudioManager.STREAM_MUSIC, context.sampleRate, AudioFormat.CHANNEL_OUT_MONO,
      AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM
    )
  }

  external fun initHybrid(): HybridData?

  fun getWaveType(): String {
    return WaveType.toString(waveType)
  }

  fun setWaveType(type: String) {
    waveType = WaveType.fromString(type)
  }

  override fun start(time: Double) {
    if(isPlaying) {
      return
    }

    playbackThread = Thread {
      try {
        Thread.sleep((time * 1000).toLong())
      } catch (e: InterruptedException) {
        Log.e("OscillatorNode", "Thread sleep error: ${e.message}")
      }

      isPlaying = true
      audioTrack.play()
      generateSound()
    }.apply { start() }
  }

  override fun stop(time: Double) {
    if(!isPlaying) {
      return
    }

    stopThread = Thread {
      try {
        Thread.sleep((time * 1000).toLong())
      } catch (e: InterruptedException) {
        Log.e("OscillatorNode", "Thread sleep error: ${e.message}")
      }

      isPlaying = false
      audioTrack.stop()
      playbackThread?.join()
    }.apply { start() }

    stopThread?.join()
  }

  private fun generateSound() {
    var wavePhase = 0.0
    var phaseChange: Double

    while(isPlaying) {
      phaseChange = 2 * Math.PI * (frequency + detune) / context.sampleRate

      for(i in buffer.indices) {
        buffer[i] = WaveType.getWaveBufferElement(wavePhase, waveType)
        wavePhase += phaseChange
      }
      process(buffer, audioTrack)
    }
    audioTrack.flush()
  }
}

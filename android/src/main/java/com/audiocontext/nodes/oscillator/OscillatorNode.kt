package com.audiocontext.nodes.oscillator

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.util.Log
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.parameters.AudioParam
import com.audiocontext.nodes.AudioScheduledSourceNode
import com.audiocontext.nodes.parameters.PlaybackParameters
import com.facebook.jni.HybridData

class OscillatorNode(context: BaseAudioContext) : AudioScheduledSourceNode(context) {
  override val numberOfInputs: Int = 0
  override val numberOfOutputs: Int = 1
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

  private var playbackParameters: PlaybackParameters
  @Volatile private var isPlaying: Boolean = false
  private var playbackThread: Thread? = null
  private var stopThread: Thread? = null

  private val mHybridData: HybridData? = initHybrid();

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    val bufferSize = AudioTrack.getMinBufferSize(
      context.sampleRate,
      AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT)

    val audioAttributes = AudioAttributes.Builder()
      .setUsage(AudioAttributes.USAGE_MEDIA)
      .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
      .build()

    val audioFormat = AudioFormat.Builder()
      .setSampleRate(context.sampleRate)
      .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
      .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
      .build()

    val audioTrack = AudioTrack(audioAttributes, audioFormat, bufferSize, AudioTrack.MODE_STREAM, AudioManager.AUDIO_SESSION_ID_GENERATE)
    val buffer = ShortArray(bufferSize)

    this.playbackParameters = PlaybackParameters(audioTrack, buffer)
  }

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
      playbackParameters.audioTrack.play()
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
      playbackParameters.audioTrack.stop()
      playbackThread?.join()
    }.apply { start() }

    stopThread?.join()
  }

  private fun generateSound() {
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

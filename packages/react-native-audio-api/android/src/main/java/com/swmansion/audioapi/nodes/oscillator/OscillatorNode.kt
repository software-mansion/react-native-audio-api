// package com.swmansion.audioapi.nodes.oscillator
//
// import android.os.Build
// import androidx.annotation.RequiresApi
// import com.swmansion.audioapi.context.BaseAudioContext
// import com.swmansion.audioapi.nodes.AudioScheduledSourceNode
// import com.swmansion.audioapi.parameters.AudioParam
// import com.swmansion.audioapi.parameters.PlaybackParameters
// import com.swmansion.audioapi.utils.AudioBuffer
// import com.swmansion.audioapi.utils.Constants
// import kotlin.math.pow
//
// class OscillatorNode(
//  context: BaseAudioContext,
// ) : AudioScheduledSourceNode(context) {
//  override var playbackParameters: PlaybackParameters?
//
//  private val frequency: AudioParam = AudioParam(context, 440.0, Constants.NYQUIST_FREQUENCY, -Constants.NYQUIST_FREQUENCY)
//    get() = field
//  private val detune: AudioParam = AudioParam(context, 0.0, Constants.MAX_DETUNE, -Constants.MAX_DETUNE)
//    get() = field
//  private var waveType: WaveType = WaveType.SINE
//  private var wavePhase: Double = 0.0
//
//  init {
//    val audioTrack = context.getAudioTrack(2 * Constants.BUFFER_SIZE)
//    val audioBuffer = AudioBuffer(2, Constants.BUFFER_SIZE, context.sampleRate)
//
//    playbackParameters = PlaybackParameters(audioTrack, audioBuffer)
//  }
//
//  fun getWaveType(): String = WaveType.toString(waveType)
//
//  fun setWaveType(type: String) {
//    waveType = WaveType.fromString(type)
//  }
//
//  @RequiresApi(Build.VERSION_CODES.N)
//  override fun fillBuffer(playbackParameters: PlaybackParameters) {
//    mixBuffers(playbackParameters)
//
//    for (i in 0 until playbackParameters.audioBuffer.length) {
//      val computedFrequency =
//        frequency.getValueAtTime(context.getCurrentTime()) * 2.0.pow(detune.getValueAtTime(context.getCurrentTime()) / 1200.0)
//      wavePhase += 2.0 * Math.PI * (computedFrequency / context.sampleRate)
//
//      if (wavePhase > 2.0 * Math.PI) {
//        wavePhase -= 2.0 * Math.PI
//      }
//      for (j in 0 until playbackParameters.audioBuffer.numberOfChannels) {
//        playbackParameters.audioBuffer.getChannelData(j)[i] = WaveType.getWaveBufferElement(wavePhase, waveType)
//      }
//    }
//  }
// }

package com.audiocontext.nodes

import android.media.AudioTrack
import android.util.Log
import com.audiocontext.context.BaseAudioContext
import com.facebook.jni.HybridData
import com.facebook.react.bridge.ReactApplicationContext


class AudioDestinationNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs = 1
  override val numberOfOutputs = 0

  private val mHybridData: HybridData? = initHybrid();

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  override fun process(buffer: ShortArray, audioTrack: AudioTrack) {
    audioTrack.write(buffer, 0, buffer.size)
  }
}

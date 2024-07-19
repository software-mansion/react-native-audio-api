package com.audiocontext.nodes

import android.media.AudioTrack
import com.audiocontext.context.BaseAudioContext
import com.facebook.jni.HybridData
import com.facebook.react.bridge.ReactApplicationContext


class AudioDestinationNode(context: BaseAudioContext, reactContext: ReactApplicationContext): AudioNode(context) {
  override val numberOfInputs = 1
  override val numberOfOutputs = 0

  private val mHybridData: HybridData?;

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid(reactContext.javaScriptContextHolder!!.get())
  }

  external fun initHybrid(l: Long): HybridData?

  override fun process(buffer: ShortArray, audioTrack: AudioTrack) {
    audioTrack.write(buffer, 0, buffer.size)
  }
}

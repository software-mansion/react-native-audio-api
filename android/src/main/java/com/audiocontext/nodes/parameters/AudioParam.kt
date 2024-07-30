package com.audiocontext.nodes.parameters

import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.facebook.jni.HybridData
import java.util.Optional
import java.util.PriorityQueue

class AudioParam(val context: BaseAudioContext, defaultValue: Double, maxValue: Double, minValue: Double) {
  private var value: Double = defaultValue
  private val defaultValue: Double = defaultValue
    get() = field
  private val maxValue: Double = maxValue
    get() = field
  private val minValue: Double = minValue
    get() = field

  @RequiresApi(Build.VERSION_CODES.N)
  private val changeQueue: PriorityQueue<ParamChange> = PriorityQueue(ParamChangeComparator)
  @RequiresApi(Build.VERSION_CODES.N)
  private var currentChange = Optional.empty<ParamChange>()

  private val mHybridData: HybridData?;

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid()
  }

  external fun initHybrid(): HybridData?

  fun getValue(): Double {
    return value
  }

  @RequiresApi(Build.VERSION_CODES.N)
  fun getValueAtTime(time: Double): Double {
    if(changeQueue.isNotEmpty()) {
      if(!currentChange.isPresent || currentChange.get().endTime < time){ //when the current change is over or there is no current change
        currentChange = changeQueue.poll()?.let { Optional.of(it) }!!

        if(currentChange.get().startTime >= time) {
          this.value = currentChange.get().getValueAtTime(time)
        }
      } else { //when the current change is still active and the time is within the change
        if(currentChange.get().startTime >= time) {
          this.value = currentChange.get().getValueAtTime(time)
        }
      }
    }

    return this.value
  }

  fun setValue(value: Double) {
    this.value = checkValue(value)
  }

  @RequiresApi(Build.VERSION_CODES.N)
  fun setValueAtTime(value: Double, time: Double) {
    val endValue = checkValue(value)
    val calculateValue = { _: Double, _: Double, _: Double, endValueF: Double, _: Double -> endValueF }
    val paramChange = ParamChange(time, time, endValue, endValue, calculateValue)
    changeQueue.add(paramChange)
  }

  @RequiresApi(Build.VERSION_CODES.N)
  fun linearRampToValueAtTime(value: Double, time: Double) {
    val endValue = checkValue(value)
    val calculateValue = { startTime: Double, endTime: Double, startValue: Double, endValueF: Double, timeF: Double ->
      startValue + (endValueF - startValue) * (timeF - startTime) / (endTime - startTime)
    }

    val paramChange = ParamChange(getStartTime(), time, getStartValue(), endValue, calculateValue)
    changeQueue.add(paramChange)
  }

  private fun checkValue(value: Double): Double {
    if(value > maxValue || value < minValue) {
      Log.d("AudioParam", "Value out of range, setting to default value")
      return defaultValue
    }
    return value
  }

  @RequiresApi(Build.VERSION_CODES.N)
  private fun getStartTime(): Double {
    return if (changeQueue.isEmpty()) {
      context.getCurrentTime()
    } else{
      changeQueue.last().endTime
    }
  }

  @RequiresApi(Build.VERSION_CODES.N)
  private fun getStartValue(): Double {
    return if (changeQueue.isEmpty()) {
      this.value
    } else{
      changeQueue.last().endValue
    }
  }
}

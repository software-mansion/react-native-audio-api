package com.swmansion.audioapi.system

import android.Manifest
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Build
import android.os.SystemClock
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.content.ContextCompat
import java.lang.ref.WeakReference

/**
 * The Bluetooth SCO link a headset mic carries its audio over.
 *
 * The platform brings that link up for the communication device, never for a capture
 * stream's preferred device. Without it the stream opens on the headset and receives no
 * frames, so a recording on a headset mic has to bring the link up itself.
 */
class BluetoothScoLink(
  private val audioManager: AudioManager,
  private val context: WeakReference<out Context>,
) {
  /**
   * Whether this instance started the link, so it only tears down its own.
   * Volatile: recorder threads and the main thread both read and write it.
   */
  @Volatile
  private var started = false

  /**
   * Whether the link came up since [started] was set. A loss is only reported for a link that
   * was up, so a headset the platform keeps refusing is retried once per connection, not on
   * every refusal.
   */
  @Volatile
  private var connected = false

  /** Runs when the link drops while [started]; see [startObserving]. */
  private var onLinkLost: (() -> Unit)? = null

  /** Android 12+: the communication device the platform routes to. */
  private var communicationDeviceListener: AudioManager.OnCommunicationDeviceChangedListener? = null

  /** Before Android 12: the SCO state broadcast. */
  private var scoStateReceiver: BroadcastReceiver? = null

  /**
   * Watches the link and calls [onLinkLost] when the platform routes communication away from
   * the headset while this instance still holds the link up. Listeners fire on the main thread.
   */
  fun startObserving(onLinkLost: () -> Unit) {
    this.onLinkLost = onLinkLost
    val context = this.context.get() ?: return

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      val listener =
        AudioManager.OnCommunicationDeviceChangedListener { device ->
          onCommunicationRouteChanged(scoRouted = device?.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO)
        }
      audioManager.addOnCommunicationDeviceChangedListener(ContextCompat.getMainExecutor(context), listener)
      communicationDeviceListener = listener
    } else {
      val receiver =
        object : BroadcastReceiver() {
          @Suppress("DEPRECATION")
          override fun onReceive(
            context: Context,
            intent: Intent,
          ) {
            when (intent.getIntExtra(AudioManager.EXTRA_SCO_AUDIO_STATE, AudioManager.SCO_AUDIO_STATE_ERROR)) {
              AudioManager.SCO_AUDIO_STATE_CONNECTED -> onCommunicationRouteChanged(scoRouted = true)
              AudioManager.SCO_AUDIO_STATE_DISCONNECTED -> onCommunicationRouteChanged(scoRouted = false)
            }
          }
        }

      // A system broadcast, so the receiver has to be exported on Android 14+.
      @Suppress("DEPRECATION")
      val filter = IntentFilter(AudioManager.ACTION_SCO_AUDIO_STATE_UPDATED)
      ContextCompat.registerReceiver(context, receiver, filter, ContextCompat.RECEIVER_EXPORTED)
      scoStateReceiver = receiver
    }
  }

  fun stopObserving() {
    onLinkLost = null

    communicationDeviceListener?.let {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        audioManager.removeOnCommunicationDeviceChangedListener(it)
      }
    }
    communicationDeviceListener = null

    scoStateReceiver?.let { receiver -> context.get()?.unregisterReceiver(receiver) }
    scoStateReceiver = null
  }

  private fun onCommunicationRouteChanged(scoRouted: Boolean) {
    if (!started) {
      return
    }

    if (scoRouted) {
      connected = true
      return
    }

    if (connected) {
      connected = false
      onLinkLost?.invoke()
    }
  }

  /**
   * Brings the link up for [headsetMic] and waits for it.
   *
   * Blocks for up to [CONNECT_TIMEOUT_MS], so never call it on the main thread.
   *
   * @return false when the link could not be requested or did not come up in time; the
   *   request stays in place until [tearDown] so a late connection is not left dangling.
   */
  @RequiresApi(Build.VERSION_CODES.M)
  fun bringUp(headsetMic: AudioDeviceInfo): Boolean {
    connected = start(headsetMic) && awaitConnected()
    return connected
  }

  /** Releases a link this instance started; does nothing otherwise. */
  fun tearDown() {
    if (!started) {
      return
    }

    // Cleared first, so the route change this causes is not taken for a lost link.
    started = false
    connected = false

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      audioManager.clearCommunicationDevice()
    } else {
      @Suppress("DEPRECATION")
      audioManager.isBluetoothScoOn = false
      @Suppress("DEPRECATION")
      audioManager.stopBluetoothSco()
    }
  }

  @RequiresApi(Build.VERSION_CODES.M)
  private fun start(headsetMic: AudioDeviceInfo): Boolean {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      val scoOutputs =
        audioManager.availableCommunicationDevices.filter {
          it.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO
        }
      val headset = scoOutputs.firstOrNull { it.address == headsetMic.address } ?: scoOutputs.firstOrNull()

      if (headset == null || !audioManager.setCommunicationDevice(headset)) {
        Log.w(TAG, "Cannot route communication to the Bluetooth headset, its mic stays silent")
        return false
      }
    } else {
      // Before Android 12 the request below is silently ignored without this permission,
      // and waiting for a link that never comes up would only delay the recording.
      if (!hasPermission(Manifest.permission.MODIFY_AUDIO_SETTINGS)) {
        Log.w(TAG, "MODIFY_AUDIO_SETTINGS is not granted, cannot use the Bluetooth headset mic")
        return false
      }

      @Suppress("DEPRECATION")
      audioManager.startBluetoothSco()
      @Suppress("DEPRECATION")
      audioManager.isBluetoothScoOn = true
    }

    started = true
    return true
  }

  private fun awaitConnected(): Boolean {
    val deadline = SystemClock.elapsedRealtime() + CONNECT_TIMEOUT_MS

    while (!isConnected()) {
      if (SystemClock.elapsedRealtime() >= deadline) {
        Log.w(TAG, "Bluetooth SCO link did not come up in time, the headset mic stays silent")
        return false
      }

      Thread.sleep(POLL_INTERVAL_MS)
    }

    return true
  }

  private fun isConnected(): Boolean {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      return audioManager.communicationDevice?.type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO
    }

    // The state broadcast is sticky, so the last one can be read without a receiver.
    val lastScoState =
      context.get()?.registerReceiver(null, IntentFilter(AudioManager.ACTION_SCO_AUDIO_STATE_UPDATED))

    return lastScoState?.getIntExtra(AudioManager.EXTRA_SCO_AUDIO_STATE, AudioManager.SCO_AUDIO_STATE_ERROR) ==
      AudioManager.SCO_AUDIO_STATE_CONNECTED
  }

  private fun hasPermission(permission: String): Boolean {
    val context = this.context.get() ?: return false
    return ContextCompat.checkSelfPermission(context, permission) == PackageManager.PERMISSION_GRANTED
  }

  private companion object {
    const val TAG = "BluetoothScoLink"
    const val CONNECT_TIMEOUT_MS = 3000L
    const val POLL_INTERVAL_MS = 50L
  }
}

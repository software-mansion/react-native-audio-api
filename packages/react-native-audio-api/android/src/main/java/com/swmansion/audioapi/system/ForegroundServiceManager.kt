package com.swmansion.audioapi.system

import android.content.Intent
import android.os.Build
import android.util.Log
import com.facebook.react.bridge.ReactApplicationContext
import com.swmansion.audioapi.system.notification.BaseNotification
import java.lang.ref.WeakReference

/**
 * Centralized manager for foreground service lifecycle.
 * Handles starting/stopping foreground service based on active subscribers.
 *
 * Subscribe and unsubscribe run on the caller's thread (the JS thread, or the recording
 * notification receiver's executor), while the service's own callbacks run on the main
 * thread. The manager therefore tracks what it asked for ([startIntentSent]) separately
 * from what the service reports ([isServiceRunning]), and never infers one from the other.
 */
object ForegroundServiceManager {
  private const val TAG = "ForegroundServiceManager"

  private lateinit var reactContext: WeakReference<ReactApplicationContext>
  private val subscribers = mutableSetOf<BaseNotification>()
  private var isServiceRunning = false
  private var startIntentSent = false

  /** Set while a restart triggered by [onServiceDestroyed] is in flight, so a service that
   * never comes up cannot be restarted in a loop. */
  private var restartAfterDestroyPending = false

  fun initialize(reactContext: WeakReference<ReactApplicationContext>) {
    this.reactContext = reactContext
  }

  /**
   * Subscribe to foreground service. Service will start if not already running.
   * @param subscriber Unique identifier for the subscriber
   */
  @Synchronized
  fun subscribe(subscriber: BaseNotification) {
    if (subscribers.add(subscriber)) {
      Log.d(TAG, "Subscriber added: $subscriber (total: ${subscribers.size})")
      startServiceIfNeeded()
    }
  }

  /**
   * Unsubscribe from foreground service. Service will stop if no more subscribers.
   * @param subscriber Unique identifier for the subscriber
   */
  @Synchronized
  fun unsubscribe(subscriber: BaseNotification) {
    if (subscribers.remove(subscriber)) {
      Log.d(TAG, "Subscriber removed: $subscriber (total: ${subscribers.size})")
      stopServiceIfNotNeeded()
    }
  }

  /**
   * Get count of active subscribers
   */
  @Synchronized
  fun getSubscriberCount(): Int = subscribers.size

  /**
   * Check if service is currently running
   */
  @Synchronized
  fun isServiceRunning(): Boolean = isServiceRunning

  /**
   * Called from [CentralizedForegroundService.onCreate].
   */
  @Synchronized
  internal fun onServiceCreated() {
    isServiceRunning = true
    restartAfterDestroyPending = false
  }

  @Synchronized
  internal fun onServiceDestroyed() {
    isServiceRunning = false

    val stillWanted = startIntentSent && subscribers.isNotEmpty() && !restartAfterDestroyPending
    // The instance is gone, so the previous request is spent either way: anything that
    // needs the service from here on has to ask for it again.
    startIntentSent = false

    if (!stillWanted) {
      return
    }

    Log.w(TAG, "Service destroyed while ${subscribers.size} subscriber(s) still need it, restarting")
    restartAfterDestroyPending = true
    startForegroundService()
  }

  private fun startServiceIfNeeded() {
    if (!startIntentSent && subscribers.isNotEmpty()) {
      startForegroundService()
    }
  }

  private fun stopServiceIfNotNeeded() {
    if ((startIntentSent || isServiceRunning) && subscribers.isEmpty()) {
      stopForegroundService()
    }
  }

  private fun startForegroundService() {
    val context = reactContext.get() ?: return

    try {
      val intent = Intent(context, CentralizedForegroundService::class.java)
      intent.action = CentralizedForegroundService.ACTION_START

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        context.startForegroundService(intent)
      } else {
        context.startService(intent)
      }

      startIntentSent = true
      Log.d(TAG, "Centralized foreground service requested to start")
    } catch (e: Exception) {
      Log.e(TAG, "Error starting foreground service: ${e.message}", e)
    }
  }

  private fun stopForegroundService() {
    startIntentSent = false

    val context = reactContext.get() ?: return

    try {
      val intent = Intent(context, CentralizedForegroundService::class.java)
      intent.action = CentralizedForegroundService.ACTION_STOP

      context.startService(intent)
      Log.d(TAG, "Centralized foreground service requested to stop")
    } catch (e: Exception) {
      Log.e(TAG, "Error stopping foreground service: ${e.message}", e)
    }
  }

  /**
   * Cleanup all subscribers and stop service
   */
  @Synchronized
  fun cleanup() {
    subscribers.clear()
    stopServiceIfNotNeeded()
  }
}

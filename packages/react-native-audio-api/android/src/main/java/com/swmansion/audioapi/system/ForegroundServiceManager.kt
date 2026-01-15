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
 */
object ForegroundServiceManager {
  private const val TAG = "ForegroundServiceManager"

  private lateinit var reactContext: WeakReference<ReactApplicationContext>
  private val subscribers = mutableSetOf<BaseNotification>()
  private var isServiceRunning = false

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
  fun getSubscriberCount(): Int = subscribers.size

  /**
   * Check if service is currently running
   */
  fun isServiceRunning(): Boolean = isServiceRunning

  private fun startServiceIfNeeded() {
    if (!isServiceRunning && subscribers.isNotEmpty()) {
      startForegroundService()
    }
  }

  private fun stopServiceIfNotNeeded() {
    if (isServiceRunning && subscribers.isEmpty()) {
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

      isServiceRunning = true
      Log.d(TAG, "Centralized foreground service started")
    } catch (e: Exception) {
      Log.e(TAG, "Error starting foreground service: ${e.message}", e)
    }
  }

  private fun stopForegroundService() {
    val context = reactContext.get() ?: return

    try {
      val intent = Intent(context, CentralizedForegroundService::class.java)
      intent.action = CentralizedForegroundService.ACTION_STOP

      context.startService(intent)
      isServiceRunning = false
      Log.d(TAG, "Centralized foreground service stopped")
    } catch (e: Exception) {
      Log.e(TAG, "Error stopping foreground service: ${e.message}", e)
    }
  }

  /**
   * Cleanup all subscribers and stop service
   */
  fun cleanup() {
    synchronized(this) {
      subscribers.clear()
      if (isServiceRunning) {
        stopForegroundService()
      }
    }
  }
}

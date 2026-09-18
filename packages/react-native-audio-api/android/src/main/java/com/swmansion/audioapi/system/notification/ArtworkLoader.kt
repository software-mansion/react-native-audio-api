package com.swmansion.audioapi.system.notification

import android.graphics.Bitmap
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.util.Log
import com.facebook.common.executors.CallerThreadExecutor
import com.facebook.common.references.CloseableReference
import com.facebook.datasource.DataSource
import com.facebook.drawee.backends.pipeline.Fresco
import com.facebook.imagepipeline.common.ImageDecodeOptions
import com.facebook.imagepipeline.common.ResizeOptions
import com.facebook.imagepipeline.core.ImagePipeline
import com.facebook.imagepipeline.datasource.BaseBitmapDataSubscriber
import com.facebook.imagepipeline.image.CloseableImage
import com.facebook.imagepipeline.request.ImageRequestBuilder
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.modules.fresco.FrescoModule
import java.lang.ref.WeakReference
import java.util.concurrent.atomic.AtomicBoolean

private typealias ImageDataSource = DataSource<CloseableReference<CloseableImage>>

/**
 * Loads media notification artwork through React Native's Fresco image pipeline.
 */
class ArtworkLoader(
  private val reactContext: WeakReference<ReactApplicationContext>,
) {
  /** Abandons a load. Idempotent, non-blocking, and safe to call from any thread. */
  fun interface Handle {
    fun cancel()
  }

  companion object {
    private const val TAG = "ArtworkLoader"

    /** Deadline covering connect, download and decode together. */
    private const val ARTWORK_FETCH_TIMEOUT_MS = 10_000L
  }

  private val mainHandler = Handler(Looper.getMainLooper())

  /**
   * Latches once Fresco has been found to be unusable, so that a host app without it does not pay
   * for a failing module lookup on every metadata update.
   */
  @Volatile
  private var isPipelineUnavailable = false

  /**
   * Fetches [uri], decoded no smaller than [targetSizePx] per side.
   */
  fun load(
    uri: Uri,
    targetSizePx: Int,
    onResult: (Bitmap?) -> Unit,
  ): Handle {
    val imagePipeline = imagePipelineOrNull() ?: return deliverNothing(onResult)

    val request =
      ImageRequestBuilder
        .newBuilderWithSource(uri)
        .setResizeOptions(ResizeOptions(targetSizePx, targetSizePx))
        // forcing first frame of animated image
        .setImageDecodeOptions(
          ImageDecodeOptions
            .newBuilder()
            .setForceStaticImage(true)
            .build(),
        ).build()

    return Fetch(imagePipeline.fetchDecodedImage(request, null), uri, onResult).also { it.start() }
  }

  private fun deliverNothing(onResult: (Bitmap?) -> Unit): Handle {
    postToNativeModulesQueue { onResult(null) }
    return Handle {}
  }

  private fun postToNativeModulesQueue(action: () -> Unit) {
    reactContext.get()?.runOnNativeModulesQueueThread { action() }
  }

  /**
   * Returns React Native's shared image pipeline, or null when Fresco is unavailable.
   *
   * `Fresco.initialize` must never be called here: [FrescoModule] owns it and supplies the pipeline
   * configuration mounted image views depend on, and a second call rebuilds the pipeline and orphans
   * the previous one with its caches. The module is a lazily created TurboModule, so requesting it
   * from the React context runs that initialization along the same path mounting an image would.
   */
  private fun imagePipelineOrNull(): ImagePipeline? {
    if (isPipelineUnavailable) return null

    if (!Fresco.hasBeenInitialized()) {
      val context = reactContext.get() ?: return null
      try {
        context.getNativeModule(FrescoModule::class.java)
      } catch (e: Exception) {
        Log.w(TAG, "Could not obtain FrescoModule: ${e.message}")
      }

      if (!Fresco.hasBeenInitialized()) {
        isPipelineUnavailable = true
        Log.w(TAG, "Fresco is not initialized; notification artwork will not be loaded")
        return null
      }
    }

    return Fresco.getImagePipeline()
  }

  /**
   * A single in-flight fetch. Success, failure, timeout and cancellation race each other, and
   * [hasDelivered] makes the outcome exactly-once.
   */
  private inner class Fetch(
    private val dataSource: ImageDataSource,
    private val uri: Uri,
    private val onResult: (Bitmap?) -> Unit,
  ) : Handle {
    private val hasDelivered = AtomicBoolean(false)

    private val onTimeout =
      Runnable {
        Log.w(TAG, "Artwork fetch for $uri exceeded $ARTWORK_FETCH_TIMEOUT_MS ms, cancelling")
        settle(null, notify = true)
      }

    fun start() {
      mainHandler.postDelayed(onTimeout, ARTWORK_FETCH_TIMEOUT_MS)
      dataSource.subscribe(
        object : BaseBitmapDataSubscriber() {
          override fun onNewResultImpl(bitmap: Bitmap?) {
            settle(bitmap?.copy(Bitmap.Config.ARGB_8888, false), notify = true)
          }

          override fun onFailureImpl(failedDataSource: ImageDataSource) {
            Log.w(TAG, "Failed to load artwork from $uri: ${failedDataSource.failureCause?.message}")
            settle(null, notify = true)
          }
        },
        CallerThreadExecutor.getInstance(),
      )
    }

    // do not notify, so canceled request do not override current artwork
    override fun cancel() = settle(null, notify = false)

    private fun settle(
      bitmap: Bitmap?,
      notify: Boolean,
    ) {
      if (!hasDelivered.compareAndSet(false, true)) return
      mainHandler.removeCallbacks(onTimeout)
      // Closing aborts the fetch; closing an already finished source is harmless.
      dataSource.close()
      if (notify) {
        postToNativeModulesQueue { onResult(bitmap) }
      }
    }
  }
}

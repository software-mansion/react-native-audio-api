#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <worklets/Compat/StableApi.h>

#include <jsi/jsi.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace audioworklets {

using namespace facebook;

/// @brief Owns a fixed pool of per-channel `AudioArrayBuffer`s and stable `Float32Array[]`
/// views over that memory on a worklet runtime. Native code writes samples into the pool;
/// JavaScript reads them through the pre-built views (zero-copy).
class AudioChannelViews {
 public:
  /// @brief Allocates `channelCount` native buffers of `frameCount` frames and builds JSI views on `runtime`.
  /// @param runtime Worklet runtime on which all JSI values are created.
  /// @param frameCount Number of frames per channel (view length).
  /// @param channelCount Number of channel slots in the pool.
  AudioChannelViews(
      const std::shared_ptr<worklets::WorkletRuntime> &runtime,
      size_t frameCount,
      size_t channelCount);

  DELETE_COPY_AND_MOVE(AudioChannelViews);
  ~AudioChannelViews();

  /// @brief Returns a stable `Float32Array[]` whose length equals `activeChannelCount`,
  /// or `nullptr` if the views were released or `activeChannelCount` is out of range.
  /// @param activeChannelCount Number of active channels for this callback.
  [[nodiscard]] const jsi::Value *channelsArray(size_t activeChannelCount) const;

  /// @brief Native per-channel buffer for audio-thread reads/writes.
  /// @note Audio Thread only. Do not access from JS.
  /// @param channelIndex Zero-based channel index.
  [[nodiscard]] const std::shared_ptr<audioapi::AudioArrayBuffer> &channelBuffer(
      size_t channelIndex) const;

  [[nodiscard]] size_t channelCount() const {
    return channelBuffers_.size();
  }

  [[nodiscard]] size_t frameCount() const {
    return frameCount_;
  }

  /// @brief Destroys JSI views on the worklet runtime. Safe to call multiple times.
  void releaseJsValues();

 private:
  void createFloat32ChannelViews(jsi::Runtime &rt);
  void createChannelsArraysByCount(jsi::Runtime &rt);

  /// Keeps the worklet runtime alive until JSI views are released.
  std::shared_ptr<worklets::WorkletRuntime> runtime_;
  bool jsValuesReleased_{false};
  size_t frameCount_{0};
  std::vector<std::shared_ptr<audioapi::AudioArrayBuffer>> channelBuffers_;
  std::vector<jsi::Value> arrayBufferValues_;
  std::vector<jsi::Value> float32ChannelValues_;
  /// @brief `channelsArraysByCount_[n]` is a `Float32Array[]` of length `n` (index 0 unused).
  std::vector<jsi::Value> channelsArraysByCount_;
};

} // namespace audioworklets

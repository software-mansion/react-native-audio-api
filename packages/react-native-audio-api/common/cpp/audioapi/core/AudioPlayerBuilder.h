#pragma once

#include <audioapi/core/CommonPlayer.h>
#include <audioapi/core/types/AudioContextOptions.h>

#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace audioapi {

class AudioContext;
class AudioPlayerBuilder;

template <typename Player>
concept BuildableAudioPlayer = std::derived_from<Player, CommonPlayer> &&
    std::constructible_from<Player, const AudioPlayerBuilder &>;

class AudioPlayerBuilder : public CommonPlayerProperties {
 public:
  explicit AudioPlayerBuilder(std::atomic<uint32_t> &currentRenders)
      : CommonPlayerProperties(currentRenders) {}

  AudioPlayerBuilder &setRenderAudio(std::function<void(DSPAudioBuffer *, int)> renderAudio) {
    renderAudio_ = std::move(renderAudio);
    return *this;
  }

  AudioPlayerBuilder &setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    return *this;
  }

  AudioPlayerBuilder &setChannelCount(int channelCount) {
    channelCount_ = channelCount;
    return *this;
  }

  /// @note Android only.
  AudioPlayerBuilder &setDriverMutex(std::mutex *driverMutex) {
    driverMutex_ = driverMutex;
    return *this;
  }

  /// @note Android only.
  AudioPlayerBuilder &setContext(const std::shared_ptr<AudioContext> &context) {
    context_ = context;
    return *this;
  }

  AudioPlayerBuilder &setAndroidOutputProfile(AndroidOutputProfile androidOutputProfile) {
    androidOutputProfile_ = androidOutputProfile;
    return *this;
  }

  /// @note Android only.
  [[nodiscard]] std::mutex *getDriverMutex() const {
    return driverMutex_;
  }

  [[nodiscard]] const std::weak_ptr<AudioContext> &getContext() const {
    return context_;
  }

  [[nodiscard]] AndroidOutputProfile getAndroidOutputProfile() const {
    return androidOutputProfile_;
  }

  template <BuildableAudioPlayer Player>
  [[nodiscard]] std::shared_ptr<CommonPlayer> build() const {
    return std::make_shared<Player>(*this);
  }

 private:
  std::mutex *driverMutex_{nullptr};
  std::weak_ptr<AudioContext> context_;
  AndroidOutputProfile androidOutputProfile_{AndroidOutputProfile::Media};
};

} // namespace audioapi

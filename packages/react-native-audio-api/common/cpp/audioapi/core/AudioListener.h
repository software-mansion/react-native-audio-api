#pragma once

#include <audioapi/core/AudioParam.h>

#include <memory>

namespace audioapi {

class BaseAudioContext;

/// @brief Represents the position and orientation of the person listening to
/// the audio scene (https://webaudio.github.io/web-audio-api/#AudioListener).
class AudioListener {
 public:
  explicit AudioListener(const std::shared_ptr<BaseAudioContext> &context);

  /// @note JS Thread only
  [[nodiscard]] std::shared_ptr<AudioParam> getPositionXParam() const {
    return positionXParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getPositionYParam() const {
    return positionYParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getPositionZParam() const {
    return positionZParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getForwardXParam() const {
    return forwardXParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getForwardYParam() const {
    return forwardYParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getForwardZParam() const {
    return forwardZParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getUpXParam() const {
    return upXParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getUpYParam() const {
    return upYParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getUpZParam() const {
    return upZParam_;
  }

 private:
  std::shared_ptr<AudioParam> positionXParam_;
  std::shared_ptr<AudioParam> positionYParam_;
  std::shared_ptr<AudioParam> positionZParam_;
  std::shared_ptr<AudioParam> forwardXParam_;
  std::shared_ptr<AudioParam> forwardYParam_;
  std::shared_ptr<AudioParam> forwardZParam_;
  std::shared_ptr<AudioParam> upXParam_;
  std::shared_ptr<AudioParam> upYParam_;
  std::shared_ptr<AudioParam> upZParam_;
};

} // namespace audioapi

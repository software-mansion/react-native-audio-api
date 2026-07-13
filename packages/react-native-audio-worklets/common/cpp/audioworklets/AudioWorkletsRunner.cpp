#include <audioworklets/AudioWorkletsRunner.h>

#include <memory>
#include <utility>

namespace audioworklets {

AudioWorkletsRunner::AudioWorkletsRunner(
    std::weak_ptr<worklets::WorkletRuntime> weakRuntime,
    const std::shared_ptr<worklets::Serializable> &serializableWorklet)
    : weakRuntime_(std::move(weakRuntime)) {
  auto strongRuntime = weakRuntime_.lock();
  if (strongRuntime == nullptr) {
    return;
  }

  unsafeRuntimePtr_ = &worklets::getJSIRuntimeFromWorkletRuntime(strongRuntime);
  strongRuntime->runSync([this, serializableWorklet](jsi::Runtime &rt) -> jsi::Value {
    new (reinterpret_cast<jsi::Function *>(unsafeWorklet_.data()))
        jsi::Function(serializableWorklet->toJSValue(rt).asObject(rt).asFunction(rt));
    workletInitialized_ = true;
    return jsi::Value::undefined();
  });
}

AudioWorkletsRunner::AudioWorkletsRunner(AudioWorkletsRunner &&other) noexcept
    : weakRuntime_(std::move(other.weakRuntime_)),
      unsafeRuntimePtr_(other.unsafeRuntimePtr_),
      workletInitialized_(other.workletInitialized_) {
  if (workletInitialized_) {
    unsafeWorklet_ = other.unsafeWorklet_;
    other.workletInitialized_ = false;
    other.unsafeRuntimePtr_ = nullptr;
  }
}

AudioWorkletsRunner &AudioWorkletsRunner::operator=(AudioWorkletsRunner &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  destroyCachedWorklet();

  weakRuntime_ = std::move(other.weakRuntime_);
  unsafeRuntimePtr_ = other.unsafeRuntimePtr_;
  workletInitialized_ = other.workletInitialized_;

  if (workletInitialized_) {
    unsafeWorklet_ = other.unsafeWorklet_;
    other.workletInitialized_ = false;
    other.unsafeRuntimePtr_ = nullptr;
  }

  return *this;
}

AudioWorkletsRunner::~AudioWorkletsRunner() {
  destroyCachedWorklet();
}

void AudioWorkletsRunner::destroyCachedWorklet() {
  if (!workletInitialized_) {
    return;
  }

  auto strongRuntime = weakRuntime_.lock();
  if (strongRuntime == nullptr) {
    workletInitialized_ = false;
    unsafeRuntimePtr_ = nullptr;
    return;
  }

  strongRuntime->runSync([this](jsi::Runtime & /*rt*/) -> jsi::Value {
    reinterpret_cast<jsi::Function *>(unsafeWorklet_.data())->~Function();
    workletInitialized_ = false;
    unsafeRuntimePtr_ = nullptr;
    return jsi::Value::undefined();
  });
}

std::shared_ptr<AudioChannelViews> AudioWorkletsRunner::createChannelViews(
    size_t frameCount,
    size_t channelCount) {
  auto strongRuntime = weakRuntime_.lock();
  if (strongRuntime == nullptr) {
    return nullptr;
  }

  return std::make_shared<AudioChannelViews>(strongRuntime, frameCount, channelCount);
}

} // namespace audioworklets

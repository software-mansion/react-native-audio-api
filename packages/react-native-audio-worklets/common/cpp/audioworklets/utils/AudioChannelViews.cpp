#include <audioworklets/utils/AudioChannelViews.h>
#include <worklets/WorkletRuntime/WorkletRuntime.h>

#include <memory>
#include <stdexcept>
#include <utility>

namespace audioworklets {

AudioChannelViews::AudioChannelViews(
    const std::shared_ptr<worklets::WorkletRuntime> &runtime,
    size_t frameCount,
    size_t channelCount)
    : runtime_(runtime), frameCount_(frameCount) {
  if (runtime == nullptr || frameCount == 0 || channelCount == 0) {
    throw std::invalid_argument("AudioChannelViews: invalid construction arguments");
  }

  channelBuffers_.resize(channelCount);

  for (size_t ch = 0; ch < channelCount; ++ch) {
    channelBuffers_[ch] = std::make_shared<audioapi::AudioArrayBuffer>(frameCount);
  }

  runtime_->runSync([this](jsi::Runtime &rt) -> jsi::Value {
    createFloat32ChannelViews(rt);
    createChannelsArraysByCount(rt);
    return jsi::Value::undefined();
  });
}

AudioChannelViews::~AudioChannelViews() {
  releaseJsValues();
}

void AudioChannelViews::releaseJsValues() {
  if (jsValuesReleased_ || runtime_ == nullptr) {
    return;
  }

  runtime_->runSync([this](jsi::Runtime &) -> jsi::Value {
    arrayBufferValues_.clear();
    float32ChannelValues_.clear();
    channelsArraysByCount_.clear();
    return jsi::Value::undefined();
  });
  jsValuesReleased_ = true;
  runtime_.reset();
}

void AudioChannelViews::createFloat32ChannelViews(jsi::Runtime &rt) {
  const auto channelCount = channelBuffers_.size();
  auto float32ArrayCtor = rt.global().getPropertyAsFunction(rt, "Float32Array");

  arrayBufferValues_.resize(channelCount);
  float32ChannelValues_.resize(channelCount);

  for (size_t ch = 0; ch < channelCount; ++ch) {
    auto arrayBuffer = jsi::ArrayBuffer(rt, channelBuffers_[ch]);
    arrayBufferValues_[ch] = jsi::Value(std::move(arrayBuffer));

    auto float32Array = float32ArrayCtor
                            .callAsConstructor(
                                rt,
                                arrayBufferValues_[ch],
                                jsi::Value(0),
                                jsi::Value(static_cast<int>(frameCount_)))
                            .getObject(rt);

    float32Array.setExternalMemoryPressure(rt, channelBuffers_[ch]->size());
    float32ChannelValues_[ch] = jsi::Value(std::move(float32Array));
  }
}

void AudioChannelViews::createChannelsArraysByCount(jsi::Runtime &rt) {
  const auto maxChannelCount = float32ChannelValues_.size();
  channelsArraysByCount_.resize(maxChannelCount + 1);

  for (size_t count = 1; count <= maxChannelCount; ++count) {
    jsi::Array channels(rt, count);
    for (size_t ch = 0; ch < count; ++ch) {
      channels.setValueAtIndex(rt, ch, float32ChannelValues_[ch]);
    }
    channelsArraysByCount_[count] = jsi::Value(std::move(channels));
  }
}

const jsi::Value *AudioChannelViews::channelsArray(size_t activeChannelCount) const {
  if (jsValuesReleased_ || activeChannelCount == 0 ||
      activeChannelCount >= channelsArraysByCount_.size()) {
    return nullptr;
  }

  return &channelsArraysByCount_[activeChannelCount];
}

const std::shared_ptr<audioapi::AudioArrayBuffer> &AudioChannelViews::channelBuffer(
    size_t channelIndex) const {
  return channelBuffers_.at(channelIndex);
}

} // namespace audioworklets

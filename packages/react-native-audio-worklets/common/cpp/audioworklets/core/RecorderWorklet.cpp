#include <audioworklets/core/RecorderWorklet.h>

#include <memory>
#include <string>
#include <utility>

namespace audioworklets {

RecorderWorklet::RecorderWorklet(
    UIWorkletsRunner runner,
    WorkletNodeDomain domain,
    const WorkletNodeOptions &options)
    : snapshots_(std::move(runner), domain, options) {}

audioapi::Result<audioapi::NoneType, std::string> RecorderWorklet::prepare(
    const audioapi::StreamFormat &format) {
  mono_ = std::make_unique<audioapi::AudioBuffer>(
      format.maxFramesPerBuffer, 1, format.layout.sampleRate);
  snapshots_.reset();
  snapshots_.setSampleRate(format.layout.sampleRate);
  return audioapi::Ok(audioapi::None);
}

audioapi::Result<audioapi::NoneType, std::string> RecorderWorklet::consume(
    const audioapi::AudioBuffer &frames,
    size_t numFrames) {
  mono_->copy(frames, 0, 0, numFrames);
  snapshots_.push(mono_->getChannel(0)->begin(), numFrames);
  return audioapi::Ok(audioapi::None);
}

void RecorderWorklet::close() {
  snapshots_.reset();
  mono_.reset();
}

void RecorderWorklet::invalidate() {
  snapshots_.deactivate();
}

} // namespace audioworklets

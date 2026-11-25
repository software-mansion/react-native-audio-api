#pragma once

#include <audioapi/core/AudioConsumer.h>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// NOLINTBEGIN

namespace audioapi::test_support {

/// Records how a recorder drives it. Read its log once the session that fed it has ended, which
/// joins the worker that feeds it.
class FakeAudioConsumer final : public AudioConsumer {
 public:
  enum class Call { Prepare, Consume, Close };

  struct LoggedCall {
    Call call;
    /// Prepare: the format's channel count. Consume: the buffer's channel count.
    size_t channelCount = 0;
    /// Consume only.
    size_t numFrames = 0;
    std::thread::id thread;

    bool operator==(const LoggedCall &) const = default;
  };

  Result<NoneType, std::string> prepare(const StreamFormat &format) override {
    log(Call::Prepare, static_cast<size_t>(format.layout.channelCount), 0);
    if (failPrepare.load()) {
      return Err("Fake consumer cannot be prepared");
    }
    return Ok(None);
  }

  Result<NoneType, std::string> consume(const AudioBuffer &frames, size_t numFrames) override {
    log(Call::Consume, frames.getNumberOfChannels(), numFrames);
    if (failConsume.load()) {
      return Err("Fake consumer cannot consume");
    }
    return Ok(None);
  }

  void close() override {
    log(Call::Close, 0, 0);
  }

  [[nodiscard]] std::vector<Call> calls() const {
    std::scoped_lock lock(mutex_);
    std::vector<Call> kinds;
    for (const auto &logged : log_) {
      kinds.push_back(logged.call);
    }
    return kinds;
  }

  [[nodiscard]] std::vector<LoggedCall> consumed() const {
    std::scoped_lock lock(mutex_);
    std::vector<LoggedCall> consumedCalls;
    for (const auto &logged : log_) {
      if (logged.call == Call::Consume) {
        consumedCalls.push_back(logged);
      }
    }
    return consumedCalls;
  }

  std::atomic<bool> failPrepare{false};
  std::atomic<bool> failConsume{false};

 private:
  void log(Call call, size_t channelCount, size_t numFrames) {
    std::scoped_lock lock(mutex_);
    log_.push_back(
        LoggedCall{
            .call = call,
            .channelCount = channelCount,
            .numFrames = numFrames,
            .thread = std::this_thread::get_id(),
        });
  }

  mutable std::mutex mutex_;
  std::vector<LoggedCall> log_;
};

} // namespace audioapi::test_support

// NOLINTEND

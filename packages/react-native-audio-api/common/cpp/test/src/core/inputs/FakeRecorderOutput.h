#pragma once

#include <audioapi/core/inputs/RecorderOutput.h>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// NOLINTBEGIN

namespace audioapi::test_support {

/// Records how a recorder drives it. Read its log once the session that fed it has ended, which
/// joins the worker that writes to it.
class FakeRecorderOutput final : public RecorderOutput {
 public:
  enum class Call { Prepare, Write, Close };

  struct LoggedCall {
    Call call;
    /// Prepare: the format's channel count. Write: the buffer's channel count.
    size_t channelCount = 0;
    /// Write only.
    size_t numFrames = 0;
    std::thread::id thread;

    bool operator==(const LoggedCall &) const = default;
  };

  Result<NoneType, std::string> prepare(const StreamFormat &format) override {
    log(Call::Prepare, static_cast<size_t>(format.layout.channelCount), 0);
    if (failPrepare.load()) {
      return Err("Fake output cannot be prepared");
    }
    return Ok(None);
  }

  Result<NoneType, std::string> write(const AudioBuffer &frames, size_t numFrames) override {
    log(Call::Write, frames.getNumberOfChannels(), numFrames);
    if (failWrite.load()) {
      return Err("Fake output cannot be written");
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

  [[nodiscard]] std::vector<LoggedCall> writes() const {
    std::scoped_lock lock(mutex_);
    std::vector<LoggedCall> written;
    for (const auto &logged : log_) {
      if (logged.call == Call::Write) {
        written.push_back(logged);
      }
    }
    return written;
  }

  std::atomic<bool> failPrepare{false};
  std::atomic<bool> failWrite{false};

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

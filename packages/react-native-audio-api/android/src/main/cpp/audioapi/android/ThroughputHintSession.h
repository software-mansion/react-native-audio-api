#pragma once

#include <audioapi/utils/Macros.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

struct APerformanceHintSession;

namespace audioapi::android {

/// ADPF performance hint session that keeps CPU clocks raised for a group of
/// threads doing bursty work which would otherwise never look busy enough to
/// the governor. Requires API 33; ThroughputHintSession.cpp weak-links the NDK
/// symbols and guards every call.
class ThroughputHintSession {
 public:
  /// Returns null below API 33 or when the device offers no hint sessions.
  [[nodiscard]] static std::unique_ptr<ThroughputHintSession> create(
      const std::vector<int32_t> &threadIds);

  ~ThroughputHintSession() = default;
  DELETE_COPY_AND_MOVE(ThroughputHintSession);

  /// Reports how long one burst of work took on the session's threads.
  void reportWork(std::chrono::nanoseconds duration);

 private:
  struct SessionCloser {
    void operator()(APerformanceHintSession *session) const;
  };
  using SessionPtr = std::unique_ptr<APerformanceHintSession, SessionCloser>;

  explicit ThroughputHintSession(SessionPtr session);

  SessionPtr session_;
};

} // namespace audioapi::android

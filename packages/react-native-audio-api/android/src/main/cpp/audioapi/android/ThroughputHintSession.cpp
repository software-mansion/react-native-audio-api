#include <audioapi/android/ThroughputHintSession.h>

#include <android/performance_hint.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi::android {
namespace {

// Shorter than any burst on purpose: every report then asks for more CPU,
// keeping the clocks raised for as long as the session lives.
constexpr int64_t THROUGHPUT_HINT_TARGET_NS = 1'000'000;

} // namespace

std::unique_ptr<ThroughputHintSession> ThroughputHintSession::create(
    const std::vector<int32_t> &threadIds) {
  if (__builtin_available(android 33, *)) {
    APerformanceHintManager *manager = APerformanceHint_getManager();
    if (manager == nullptr) {
      return nullptr;
    }
    SessionPtr session(APerformanceHint_createSession(
        manager, threadIds.data(), threadIds.size(), THROUGHPUT_HINT_TARGET_NS));
    if (session == nullptr) {
      return nullptr;
    }
    return std::unique_ptr<ThroughputHintSession>(new ThroughputHintSession(std::move(session)));
  }
  return nullptr;
}

ThroughputHintSession::ThroughputHintSession(SessionPtr session) : session_(std::move(session)) {}

void ThroughputHintSession::SessionCloser::operator()(APerformanceHintSession *session) const {
  if (__builtin_available(android 33, *)) {
    APerformanceHint_closeSession(session);
  }
}

void ThroughputHintSession::reportWork(std::chrono::nanoseconds duration) {
  if (__builtin_available(android 33, *)) {
    APerformanceHint_reportActualWorkDuration(
        session_.get(), std::max<int64_t>(duration.count(), 1));
  }
}

} // namespace audioapi::android

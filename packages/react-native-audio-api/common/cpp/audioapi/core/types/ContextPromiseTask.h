#pragma once

#include <functional>

namespace audioapi {

class BaseAudioContext;

/// @brief A lifecycle control message queued on BaseAudioContext's pending-promises
/// worker thread (spec [[pending promises]] control-plane analogue).
///
/// Default-constructed tasks are the TaskOffloader shutdown sentinel.
struct ContextPromiseTask {
  std::function<void(BaseAudioContext &)> operation;

  friend bool operator==(const ContextPromiseTask &lhs, const ContextPromiseTask &rhs) {
    return !lhs.operation && !rhs.operation;
  }
};

} // namespace audioapi

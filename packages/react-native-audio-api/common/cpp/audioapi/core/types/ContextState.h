#pragma once

#include <cstdint>

namespace audioapi {

enum class ContextState : std::uint8_t { SUSPENDED, RUNNING, CLOSED };

/// The Web Audio API `AudioContextState` string for @p state, as carried by
/// the `statechange` event payload and the JS `state` attribute.
inline const char *contextStateToString(ContextState state) {
  switch (state) {
    case ContextState::SUSPENDED:
      return "suspended";
    case ContextState::RUNNING:
      return "running";
    case ContextState::CLOSED:
      return "closed";
  }
  return "suspended";
}

} // namespace audioapi

#pragma once

#include <cstdint>

namespace audioapi {

enum class ContextState : std::uint8_t { SUSPENDED, RUNNING, CLOSED };

}

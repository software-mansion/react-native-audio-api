#pragma once

#include <cstdint>
namespace audioapi {
enum class BufferProcessingDirection : std::int8_t { FORWARD = 1, REVERSE = -1 };
} // namespace audioapi
#pragma once

#include <cstdint>
/// @brief Enum representing the direction of buffer processing, either forward or reverse.
namespace audioapi {
enum class BufferProcessingDirection : std::int8_t { FORWARD = 1, REVERSE = -1 };
} // namespace audioapi
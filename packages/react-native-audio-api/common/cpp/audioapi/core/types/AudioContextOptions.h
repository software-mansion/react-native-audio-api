#pragma once
#include <cstdint>

namespace audioapi {

enum class AndroidOutputProfile : std::uint8_t {
  Media,
  VoiceCommunication,
};

} // namespace audioapi

#pragma once

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>

#include <string>
#include <vector>

namespace audioapi {

/// Declared system-API encoding capability set for the current platform.
/// A device may still reject a format at `AudioEncoder::open()`.
class EncoderCapabilities {
 public:
  static std::vector<EncoderOutputSpec> probe();

  static bool isSupported(AudioContainer container, AudioCodec codec);

  /// Container/codec/extension mapping for a file format (ignores platform support).
  static EncoderOutputSpec specForFormat(AudioFileProperties::Format format);

  /// Maps a format to a supported output spec, or an error if unavailable here.
  static Result<EncoderOutputSpec, std::string> resolveOutputSpec(
      AudioFileProperties::Format format);
};

} // namespace audioapi

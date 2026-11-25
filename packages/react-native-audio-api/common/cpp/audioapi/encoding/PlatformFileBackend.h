#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>

#include <functional>
#include <memory>
#include <string>

namespace audioapi {

/// What a file output needs from the platform, so a test can swap the system pieces out.
struct PlatformFileBackend {
  /// Maps the requested file format to the container, codec and file extension this platform
  /// writes it as. Fails when the platform cannot encode that format, before any file exists.
  std::function<Result<EncoderOutputSpec, std::string>(AudioFileProperties::FileFormat)>
      resolveOutputSpec;
  /// Turns a bare file name (stem and extension, no directory) into the absolute path inside
  /// the directory the properties select, creating that directory if it is missing.
  std::function<Result<std::string, std::string>(
      const std::shared_ptr<AudioFileProperties> &,
      const std::string &fileName)>
      resolvePath;
  /// Builds an encoder that is not open yet; the writer opens it on the resolved path. Called
  /// once per file, so a rotating session creates one encoder per segment.
  EncoderFactory createEncoder;
};

/// The iOS and Android backend, built once and shared by every writer.
[[nodiscard]] const PlatformFileBackend &osFileBackend();

/// Creates the encoder for @p properties and opens it on a file named @p stem, in the format's
/// extension, in the directory the properties select. A name the user chose is used as given,
/// overwriting a file of that name with a warning; a generated one gets _1, _2, ... until no
/// file holds it. The opened path is the encoder's getFilePath().
[[nodiscard]] CreateEncoderResult openFileEncoder(
    const PlatformFileBackend &backend,
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &stem,
    const StreamFormat &inputFormat);

} // namespace audioapi

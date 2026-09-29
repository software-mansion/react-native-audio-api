#include <audioapi/encoding/EncoderCapabilities.h>

#include <algorithm>
#include <cstddef>
#include <string>

namespace audioapi::encoder_capabilities {

using FileFormat = AudioFileProperties::FileFormat;

EncoderOutputSpec specForFormat(FileFormat format) {
  const auto index = static_cast<size_t>(format);
  return index < kSpecsByFormat.size() ? kSpecsByFormat[index]
                                       : kSpecsByFormat[static_cast<size_t>(FileFormat::WAV)];
}

bool isSupported(AudioContainer container, AudioCodec codec) {
  return std::ranges::any_of(kSupportedFormats, [container, codec](FileFormat format) {
    const EncoderOutputSpec spec = specForFormat(format);
    return spec.container == container && spec.codec == codec;
  });
}

Result<EncoderOutputSpec, std::string> resolveOutputSpec(FileFormat format) {
  EncoderOutputSpec spec = specForFormat(format);
  if (isSupported(spec.container, spec.codec)) {
    return Result<EncoderOutputSpec, std::string>::Ok(spec);
  }

  std::string message = std::string(toString(spec.codec)) + " in " + toString(spec.container) +
      " is not encodable via system APIs on this platform. Choose a format supported by the "
      "device's system encoders.";
  return Result<EncoderOutputSpec, std::string>::Err(message);
}

} // namespace audioapi::encoder_capabilities

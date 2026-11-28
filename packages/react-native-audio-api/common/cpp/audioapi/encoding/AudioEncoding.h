#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/encoding/PlatformFileBackend.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>
#include <vector>

namespace audioapi::audioencoding {

using EncodeAudioDataResult = Result<EncodedFileInfo, std::string>;

/// Encodes @p buffers, back to back, into one file described by @p properties: its format,
/// sample rate, channel count and location. A sample rate or channel count of 0 takes the first
/// buffer's, capped at what a file can hold. Buffers may differ in sample rate and channel count;
/// the encoder resamples and remixes each to the file's. A failure leaves no file behind.
[[nodiscard]] EncodeAudioDataResult encodeAudioData(
    const std::vector<std::shared_ptr<const AudioBuffer>> &buffers,
    const std::shared_ptr<AudioFileProperties> &properties,
    const PlatformFileBackend &backend = osFileBackend());

} // namespace audioapi::audioencoding

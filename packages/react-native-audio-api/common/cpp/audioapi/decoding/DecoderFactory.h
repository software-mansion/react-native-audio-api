#pragma once

#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <audioapi/utils/Result.hpp>

#include <memory>

namespace audioapi::decoding {

using DecoderPtr = std::unique_ptr<AudioDecoderBackend>;
using CreateDecoderResult = Result<DecoderPtr, DecoderError>;

/// Selects a backend, opens @p source, and returns a ready incremental decoder.
[[nodiscard]] CreateDecoderResult createDecoder(const DecoderSource &source);

} // namespace audioapi::decoding

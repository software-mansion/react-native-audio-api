#pragma once

#include <memory>

namespace audioapi {

class AudioFileProperties;

namespace ios_encoder {

NSInteger getQuality(const std::shared_ptr<AudioFileProperties> &properties);
NSInteger getFlacCompressionLevel(const std::shared_ptr<AudioFileProperties> &properties);
NSInteger getBitDepth(const std::shared_ptr<AudioFileProperties> &properties);

} // namespace ios_encoder

} // namespace audioapi

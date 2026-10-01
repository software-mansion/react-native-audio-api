#pragma once

#include <audioapi/utils/AudioFileProperties.h>

namespace audioapi::ios::encoder {

NSInteger getQuality(const AudioFileProperties::EncodingConfig &encoding);
NSInteger getFlacCompressionLevel(const AudioFileProperties::EncodingConfig &encoding);
NSInteger getBitDepth(const AudioFileProperties::EncodingConfig &encoding);

} // namespace audioapi::ios::encoder

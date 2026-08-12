#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>

#include <cctype>
#include <string>
#include <vector>

using namespace audioapi;
using Format = AudioFileProperties::Format;

// NOLINTBEGIN

namespace {

const std::vector<Format> kAllFormats = {
    Format::WAV,
    Format::CAF,
    Format::M4A,
    Format::FLAC,
    Format::AIFF,
    Format::ALAC,
    Format::OPUS_OGG,
    Format::OPUS_WEBM,
    Format::VORBIS_WEBM,
    Format::ULAW,
    Format::ALAW,
};

} // namespace

TEST(EncoderCapabilitiesTest, SpecForFormatMapsKnownFormats) {
  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::WAV).container, AudioContainer::WAV);
  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::WAV).codec, AudioCodec::PCM);
  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::WAV).extension, "wav");

  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::M4A).container, AudioContainer::M4A);
  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::M4A).codec, AudioCodec::AAC);

  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::OPUS_WEBM).container, AudioContainer::WEBM);
  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::OPUS_WEBM).codec, AudioCodec::OPUS);

  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::FLAC).container, AudioContainer::FLAC);
  EXPECT_EQ(EncoderCapabilities::specForFormat(Format::FLAC).codec, AudioCodec::FLAC);
}

TEST(EncoderCapabilitiesTest, ExtensionsAreLowercaseAndNonEmpty) {
  for (Format format : kAllFormats) {
    const std::string ext = EncoderCapabilities::specForFormat(format).extension;
    ASSERT_FALSE(ext.empty());
    for (char c : ext) {
      EXPECT_FALSE(std::isupper(static_cast<unsigned char>(c))) << "extension: " << ext;
    }
  }
}

TEST(EncoderCapabilitiesTest, ResolveMatchesIsSupported) {
  for (Format format : kAllFormats) {
    const EncoderOutputSpec spec = EncoderCapabilities::specForFormat(format);
    const bool supported = EncoderCapabilities::isSupported(spec.container, spec.codec);
    auto resolved = EncoderCapabilities::resolveOutputSpec(format);

    EXPECT_EQ(resolved.is_ok(), supported) << "format index: " << static_cast<int>(format);
    if (resolved.is_ok()) {
      EXPECT_EQ(resolved.unwrap().container, spec.container);
      EXPECT_EQ(resolved.unwrap().codec, spec.codec);
    }
  }
}

TEST(EncoderCapabilitiesTest, ProbeEntriesAreSupported) {
  for (const auto &spec : EncoderCapabilities::probe()) {
    EXPECT_TRUE(EncoderCapabilities::isSupported(spec.container, spec.codec))
        << toString(spec.codec) << " in " << toString(spec.container);
  }
}

#if defined(__APPLE__)
TEST(EncoderCapabilitiesTest, ApplePlatformSupportsCoreFormats) {
  EXPECT_TRUE(EncoderCapabilities::isSupported(AudioContainer::WAV, AudioCodec::PCM));
  EXPECT_TRUE(EncoderCapabilities::isSupported(AudioContainer::CAF, AudioCodec::PCM));
  EXPECT_TRUE(EncoderCapabilities::isSupported(AudioContainer::M4A, AudioCodec::AAC));
  EXPECT_FALSE(EncoderCapabilities::isSupported(AudioContainer::WEBM, AudioCodec::OPUS));
}
#elif defined(__ANDROID__)
TEST(EncoderCapabilitiesTest, AndroidPlatformSupportsCoreFormats) {
  EXPECT_TRUE(EncoderCapabilities::isSupported(AudioContainer::WAV, AudioCodec::PCM));
  EXPECT_TRUE(EncoderCapabilities::isSupported(AudioContainer::M4A, AudioCodec::AAC));
  EXPECT_TRUE(EncoderCapabilities::isSupported(AudioContainer::WEBM, AudioCodec::OPUS));
  EXPECT_FALSE(EncoderCapabilities::isSupported(AudioContainer::CAF, AudioCodec::PCM));
}
#endif

// NOLINTEND

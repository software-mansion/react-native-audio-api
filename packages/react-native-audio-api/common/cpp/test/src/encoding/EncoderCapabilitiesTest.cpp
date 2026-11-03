#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

using namespace audioapi;
using FileFormat = AudioFileProperties::FileFormat;

// NOLINTBEGIN

namespace {

const std::vector<FileFormat> kAllFormats = {
    FileFormat::WAV,
    FileFormat::CAF,
    FileFormat::M4A,
    FileFormat::ADTS,
    FileFormat::FLAC,
    FileFormat::AIFF,
    FileFormat::ALAC,
    FileFormat::OPUS_OGG,
    FileFormat::OPUS_WEBM,
    FileFormat::VORBIS_WEBM,
    FileFormat::ULAW,
    FileFormat::ALAW,
};

} // namespace

TEST(EncoderCapabilitiesTest, SpecForFormatMapsKnownFormats) {
  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::WAV).container, AudioContainer::WAV);
  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::WAV).codec, AudioCodec::PCM);
  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::WAV).extension, "wav");

  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::M4A).container, AudioContainer::M4A);
  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::M4A).codec, AudioCodec::AAC);

  EXPECT_EQ(
      encoder_capabilities::specForFormat(FileFormat::OPUS_WEBM).container, AudioContainer::WEBM);
  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::OPUS_WEBM).codec, AudioCodec::OPUS);

  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::FLAC).container, AudioContainer::FLAC);
  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::FLAC).codec, AudioCodec::FLAC);

  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::ADTS).container, AudioContainer::ADTS);
  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::ADTS).codec, AudioCodec::AAC);
  EXPECT_EQ(encoder_capabilities::specForFormat(FileFormat::ADTS).extension, "aac");
}

TEST(EncoderCapabilitiesTest, ExtensionsAreLowercaseAndNonEmpty) {
  for (FileFormat format : kAllFormats) {
    const auto ext = encoder_capabilities::specForFormat(format).extension;
    ASSERT_FALSE(ext.empty());
    for (char c : ext) {
      EXPECT_FALSE(std::isupper(static_cast<unsigned char>(c))) << "extension: " << ext;
    }
  }
}

TEST(EncoderCapabilitiesTest, ResolveMatchesIsSupported) {
  for (FileFormat format : kAllFormats) {
    const EncoderOutputSpec spec = encoder_capabilities::specForFormat(format);
    const bool supported = encoder_capabilities::isSupported(spec.container, spec.codec);
    auto resolved = encoder_capabilities::resolveOutputSpec(format);

    EXPECT_EQ(resolved.is_ok(), supported) << "format index: " << static_cast<int>(format);
    if (resolved.is_ok()) {
      EXPECT_EQ(resolved.unwrap().container, spec.container);
      EXPECT_EQ(resolved.unwrap().codec, spec.codec);
    }
  }
}

TEST(EncoderCapabilitiesTest, SupportedFormatsResolve) {
  for (FileFormat format : encoder_capabilities::kSupportedFormats) {
    EXPECT_TRUE(encoder_capabilities::resolveOutputSpec(format).is_ok())
        << "format index: " << static_cast<int>(format);
  }
}

#if defined(__APPLE__)
TEST(EncoderCapabilitiesTest, ApplePlatformSupportsCoreFormats) {
  EXPECT_TRUE(encoder_capabilities::isSupported(AudioContainer::WAV, AudioCodec::PCM));
  EXPECT_TRUE(encoder_capabilities::isSupported(AudioContainer::CAF, AudioCodec::PCM));
  EXPECT_TRUE(encoder_capabilities::isSupported(AudioContainer::M4A, AudioCodec::AAC));
  EXPECT_TRUE(encoder_capabilities::isSupported(AudioContainer::ADTS, AudioCodec::AAC));
  EXPECT_FALSE(encoder_capabilities::isSupported(AudioContainer::WEBM, AudioCodec::OPUS));
}
#elif defined(__ANDROID__)
TEST(EncoderCapabilitiesTest, AndroidPlatformSupportsCoreFormats) {
  EXPECT_TRUE(encoder_capabilities::isSupported(AudioContainer::WAV, AudioCodec::PCM));
  EXPECT_TRUE(encoder_capabilities::isSupported(AudioContainer::M4A, AudioCodec::AAC));
  EXPECT_TRUE(encoder_capabilities::isSupported(AudioContainer::WEBM, AudioCodec::OPUS));
  EXPECT_TRUE(encoder_capabilities::isSupported(AudioContainer::ADTS, AudioCodec::AAC));
  EXPECT_FALSE(encoder_capabilities::isSupported(AudioContainer::CAF, AudioCodec::PCM));
}
#endif

// NOLINTEND

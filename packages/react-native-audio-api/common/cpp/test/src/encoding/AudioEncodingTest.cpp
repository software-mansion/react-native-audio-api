#include <audioapi/encoding/AudioEncoding.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr float sampleRate = 48000.0F;

/// Every input format an encoder was switched to, in order.
std::vector<StreamFormat> inputSwitches;
/// The file layout the last encoder was created with.
AudioLayout createdFileLayout;
/// What every fake encoder reports on close.
constexpr double closedSizeMB = 1.5;
constexpr double closedDurationSeconds = 2.5;

/// Writes the mono samples it was given as raw floats, so a test can read back what was encoded.
class RawFloatEncoder final : public AudioEncoder {
 public:
  RawFloatEncoder(const EncoderSettings &settings, bool refusesFrames)
      : AudioEncoder(settings), refusesFrames_(refusesFrames) {}

  OpenEncoderResult open(const StreamFormat &, const EncoderOutputSpec &, const std::string &path)
      override {
    filePath_ = path;
    std::ofstream created(path, std::ios::binary);
    markOpen();
    return OpenEncoderResult::Ok(path);
  }

  EncodeResult encode(const AudioBuffer &input, int numFrames) override {
    if (refusesFrames_) {
      return EncodeResult::Err("encoder refused the frames");
    }
    const float *mono = input.getChannel(0)->begin();
    frames_.insert(frames_.end(), mono, mono + numFrames);
    return EncodeResult::Ok(static_cast<size_t>(numFrames));
  }

  CloseEncoderResult close() override {
    std::ofstream output(filePath_, std::ios::binary);
    output.write(
        reinterpret_cast<const char *>(frames_.data()),
        static_cast<std::streamsize>(frames_.size() * sizeof(float)));
    markClosed();
    return CloseEncoderResult::Ok({closedSizeMB, closedDurationSeconds});
  }

  [[nodiscard]] size_t getFileSizeBytes() const override {
    return frames_.size() * sizeof(float);
  }

 protected:
  OpenEncoderResult reprepareInput(const StreamFormat &inputFormat) override {
    inputSwitches.push_back(inputFormat);
    return OpenEncoderResult::Ok(filePath_);
  }

 private:
  bool refusesFrames_;
  std::vector<float> frames_;
};

PlatformFileBackend makeBackend(bool refusesFrames = false) {
  return PlatformFileBackend{
      // The desktop build supports no format, so the spec is resolved without that check.
      .resolveOutputSpec =
          [](AudioFileProperties::FileFormat format) {
            return Result<EncoderOutputSpec, std::string>::Ok(
                encoder_capabilities::specForFormat(format));
          },
      .resolvePath =
          [](const std::shared_ptr<AudioFileProperties> &, const std::string &fileName) {
            return Result<std::string, std::string>::Ok(::testing::TempDir() + fileName);
          },
      .createEncoder =
          [refusesFrames](const EncoderSettings &settings) {
            createdFileLayout = settings.fileLayout;
            return CreateEncoderResult::Ok(
                std::make_unique<RawFloatEncoder>(settings, refusesFrames));
          },
  };
}

std::shared_ptr<AudioFileProperties> makeProperties(
    const std::string &fileName,
    AudioLayout fileLayout = {.sampleRate = sampleRate, .channelCount = 1}) {
  return std::make_shared<AudioFileProperties>(
      AudioFileProperties::PathConfig{
          .directory = AudioFileProperties::FileDirectory::Cache,
          .subDirectory = "",
          .fileName = fileName,
      },
      fileLayout,
      AudioFileProperties::EncodingConfig{
          .format = AudioFileProperties::FileFormat::WAV,
          .bitRate = 0,
          .bitDepth = AudioFileProperties::BitDepth::Bit16,
          .flacCompressionLevel = 0,
          .iosAudioQuality = AudioFileProperties::IOSAudioQuality::Max,
      },
      AudioFileProperties::WriterConfig{.rotateIntervalBytes = 0, .androidFlushIntervalMs = 0});
}

/// Every channel holds firstSample, firstSample + 1, ...
std::shared_ptr<const AudioBuffer>
makeBuffer(size_t frames, float firstSample, float rate = sampleRate, int channels = 1) {
  auto buffer = std::make_shared<AudioBuffer>(frames, channels, rate);
  for (int channel = 0; channel < channels; ++channel) {
    for (size_t i = 0; i < frames; ++i) {
      (*buffer->getChannel(channel))[i] = firstSample + static_cast<float>(i);
    }
  }
  return buffer;
}

/// The filesystem path of the file @p info names, without its file:// prefix.
std::string writtenPath(const EncodedFileInfo &info) {
  EXPECT_EQ(info.path.rfind("file://", 0), 0U);
  return info.path.substr(std::string_view("file://").size());
}

std::vector<float> readFloats(const std::string &path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  std::vector<float> frames(static_cast<size_t>(input.tellg()) / sizeof(float));
  input.seekg(0);
  input.read(reinterpret_cast<char *>(frames.data()), frames.size() * sizeof(float));
  return frames;
}

} // namespace

TEST(AudioEncodingTest, EncodesBuffersBackToBackAcrossChunks) {
  const auto backend = makeBackend();
  // Longer than one 4096-frame chunk, so chunking offsets are exercised.
  const auto first = makeBuffer(5000, 0.0F);
  const auto second = makeBuffer(3, 5000.0F);

  auto result = audioencoding::encodeAudioData(
      {first, second}, makeProperties("encode-back-to-back"), backend);

  ASSERT_TRUE(result.is_ok()) << result.unwrap_err();
  EXPECT_EQ(result.unwrap().size, closedSizeMB);
  EXPECT_EQ(result.unwrap().duration, closedDurationSeconds);
  const auto frames = readFloats(writtenPath(result.unwrap()));
  ASSERT_EQ(frames.size(), 5003U);
  for (size_t i = 0; i < frames.size(); ++i) {
    ASSERT_EQ(frames[i], static_cast<float>(i)) << "at frame " << i;
  }
  std::filesystem::remove(writtenPath(result.unwrap()));
}

TEST(AudioEncodingTest, SwitchesTheEncoderInputWhenTheBufferLayoutChanges) {
  const auto backend = makeBackend();
  inputSwitches.clear();

  auto result = audioencoding::encodeAudioData(
      {makeBuffer(4, 0.0F),
       makeBuffer(3, 4.0F, 44100.0F, 2),
       makeBuffer(2, 7.0F, 44100.0F, 2),
       makeBuffer(1, 9.0F)},
      makeProperties("encode-mixed-layouts"),
      backend);

  ASSERT_TRUE(result.is_ok()) << result.unwrap_err();
  // Only a layout change switches the input: the second stereo buffer reuses the first's.
  ASSERT_EQ(inputSwitches.size(), 2U);
  EXPECT_EQ(inputSwitches[0].layout, (AudioLayout{.sampleRate = 44100.0F, .channelCount = 2}));
  EXPECT_EQ(inputSwitches[1].layout, (AudioLayout{.sampleRate = sampleRate, .channelCount = 1}));
  EXPECT_EQ(
      readFloats(writtenPath(result.unwrap())),
      (std::vector<float>{0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F}));
  std::filesystem::remove(writtenPath(result.unwrap()));
}

TEST(AudioEncodingTest, CapsAnUnsetFileLayoutAtWhatAFileHolds) {
  const auto backend = makeBackend();

  auto result = audioencoding::encodeAudioData(
      {makeBuffer(4, 0.0F, 96000.0F, 6)},
      makeProperties("encode-unset-layout", {.sampleRate = 0, .channelCount = 0}),
      backend);

  ASSERT_TRUE(result.is_ok()) << result.unwrap_err();
  EXPECT_EQ(createdFileLayout, (AudioLayout{.sampleRate = 48000.0F, .channelCount = 2}));
  std::filesystem::remove(writtenPath(result.unwrap()));
}

TEST(AudioEncodingTest, RejectsEmptyInput) {
  const auto backend = makeBackend();
  auto result = audioencoding::encodeAudioData({}, makeProperties("encode-empty"), backend);
  EXPECT_TRUE(result.is_err());
}

TEST(AudioEncodingTest, LeavesNoFileWhenEncodingFails) {
  const auto backend = makeBackend(/*refusesFrames=*/true);

  auto result = audioencoding::encodeAudioData(
      {makeBuffer(4, 0.0F)}, makeProperties("encode-failing"), backend);

  ASSERT_TRUE(result.is_err());
  EXPECT_FALSE(std::filesystem::exists(::testing::TempDir() + "encode-failing.wav"));
}

// NOLINTEND

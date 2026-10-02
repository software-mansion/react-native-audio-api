#include <audioapi/core/utils/RecordingFile.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

/// Creates the file on open, as the platform encoders do; accepts or refuses every buffer.
class FakeEncoder final : public AudioEncoder {
 public:
  explicit FakeEncoder(bool acceptFrames = true, bool failOpen = false)
      : AudioEncoder(EncoderSettings{}), acceptFrames_(acceptFrames), failOpen_(failOpen) {}

  OpenEncoderResult open(
      const StreamFormat & /*inputFormat*/,
      const EncoderOutputSpec & /*outputSpec*/,
      const std::string &filePath) override {
    if (failOpen_) {
      return OpenEncoderResult::Err("encoder refused to open");
    }
    std::ofstream created(filePath, std::ios::binary);
    markOpen();
    return OpenEncoderResult::Ok(filePath);
  }

  EncodeResult encode(const float *const * /*channels*/, int numFrames) override {
    if (!acceptFrames_) {
      return EncodeResult::Err("encoder refused the frames");
    }
    return EncodeResult::Ok(static_cast<size_t>(numFrames));
  }

  CloseEncoderResult close() override {
    markClosed();
    return CloseEncoderResult::Ok({0.0, 0.0});
  }

  [[nodiscard]] size_t getFileSizeBytes() const override {
    return 0;
  }

 private:
  bool acceptFrames_;
  bool failOpen_;
};

const EncoderOutputSpec WAV_SPEC{
    .container = AudioContainer::WAV,
    .codec = AudioCodec::PCM,
    .extension = "wav"};

StreamFormat monoAt(float sampleRate) {
  return StreamFormat{
      .layout = {.sampleRate = sampleRate, .channelCount = 1}, .maxFramesPerBuffer = 4800};
}

std::string scratchPath(const std::string &name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

std::unique_ptr<RecordingFile>
openFile(std::unique_ptr<AudioEncoder> encoder, float sampleRate, const std::string &path) {
  auto result = RecordingFile::open(std::move(encoder), monoAt(sampleRate), WAV_SPEC, path);
  EXPECT_TRUE(result.is_ok());
  return std::move(result).unwrap();
}

void encodeFrames(RecordingFile &file, int numFrames) {
  std::vector<float> samples(static_cast<size_t>(numFrames), 0.0F);
  const float *channels[] = {samples.data()};
  (void)file.encode(channels, numFrames);
}

OpenEncoderResult acceptRetarget(AudioEncoder & /*encoder*/, const StreamFormat & /*format*/) {
  return OpenEncoderResult::Ok(std::string());
}

} // namespace

TEST(RecordingFileTest, CountsEncodedFramesAtTheInputRate) {
  const std::string path = scratchPath("RecordingFileTest-count.wav");
  auto file = openFile(std::make_unique<FakeEncoder>(), 48000.0F, path);

  encodeFrames(*file, 4800);

  EXPECT_DOUBLE_EQ(file->durationSec(), 0.1);
  EXPECT_EQ(file->path(), path);
  std::filesystem::remove(path);
}

TEST(RecordingFileTest, FramesTheEncoderRefusesDoNotCount) {
  const std::string path = scratchPath("RecordingFileTest-refused.wav");
  auto file = openFile(std::make_unique<FakeEncoder>(/*acceptFrames=*/false), 48000.0F, path);

  encodeFrames(*file, 4800);

  EXPECT_DOUBLE_EQ(file->durationSec(), 0.0);
  std::filesystem::remove(path);
}

TEST(RecordingFileTest, AFormatChangeKeepsEarlierAudioAtItsOwnRate) {
  const std::string path = scratchPath("RecordingFileTest-retarget.wav");
  auto file = openFile(std::make_unique<FakeEncoder>(), 48000.0F, path);

  encodeFrames(*file, 4800);
  ASSERT_TRUE(file->changeInputFormat(monoAt(24000.0F), acceptRetarget).is_ok());
  encodeFrames(*file, 2400);

  EXPECT_DOUBLE_EQ(file->durationSec(), 0.2);
  std::filesystem::remove(path);
}

TEST(RecordingFileTest, AFailedOpenYieldsNoFile) {
  auto result = RecordingFile::open(
      std::make_unique<FakeEncoder>(/*acceptFrames=*/true, /*failOpen=*/true),
      monoAt(48000.0F),
      WAV_SPEC,
      scratchPath("RecordingFileTest-refused-open.wav"));

  EXPECT_TRUE(result.is_err());
}

TEST(RecordingFileTest, DiscardDeletesTheFile) {
  const std::string path = scratchPath("RecordingFileTest-discard.wav");
  auto file = openFile(std::make_unique<FakeEncoder>(), 48000.0F, path);
  ASSERT_TRUE(std::filesystem::exists(path));

  file->discard();

  EXPECT_FALSE(std::filesystem::exists(path));
}

// NOLINTEND

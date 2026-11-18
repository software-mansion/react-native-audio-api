#include <audioapi/core/inputs/RecorderOutputs.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <test/src/core/inputs/FakeRecorderOutput.h>
#include <test/src/utils/FakeFileBackend.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace audioapi;
using namespace audioapi::test_support;

// NOLINTBEGIN

namespace {

/// Mirrors AudioBufferOffloader::POOL_SIZE: how many buffers can wait for the effects worker.
constexpr size_t kOutputsPoolSize = 32;

/// Holds the effects worker inside its first write until release() is called, as a slow output
/// would.
class StalledRecorderOutput final : public RecorderOutput {
 public:
  Result<NoneType, std::string> prepare(const StreamFormat &) override {
    return Ok(None);
  }

  Result<NoneType, std::string> write(const AudioBuffer &, size_t) override {
    if (writes_.fetch_add(1) == 0) {
      entered_.set_value();
    }
    released_.wait();
    return Ok(None);
  }

  void close() override {}

  void waitUntilStalled() {
    entered_.get_future().wait();
  }

  void release() {
    releaseSignal_.set_value();
  }

  [[nodiscard]] size_t writes() const {
    return writes_.load();
  }

 private:
  std::atomic<size_t> writes_{0};
  std::promise<void> entered_;
  std::promise<void> releaseSignal_;
  std::shared_future<void> released_{releaseSignal_.get_future().share()};
};

} // namespace

class RecorderOutputsTest : public ::testing::Test {
 protected:
  using Call = FakeRecorderOutput::Call;

  /// Feeds one buffer of FRAMES frames in @p channelCount channels, as the audio thread does.
  void writeFrames(int channelCount = 1) {
    const std::vector<float> samples(FRAMES, 0.25F);
    const std::vector<const float *> channels(channelCount, samples.data());
    outputs_.write(channels.data(), FRAMES);
  }

  /// Ends the session the way the recorder does, so every worker is joined.
  Result<FileInfo, std::string> endSession() {
    return RecorderOutputs::finalize(outputs_.detach());
  }

  std::shared_ptr<FakeRecorderOutput> addFakeOutput(
      const std::optional<StreamFormat> &currentFormat = std::nullopt) {
    auto output = std::make_shared<FakeRecorderOutput>();
    EXPECT_TRUE(outputs_.add(output, currentFormat).is_ok());
    return output;
  }

  static constexpr int FRAMES = 128;
  static constexpr StreamFormat MONO_48K{
      .layout = {.sampleRate = 48000.0F, .channelCount = 1},
      .maxFramesPerBuffer = 256,
  };
  static constexpr StreamFormat STEREO_48K{
      .layout = {.sampleRate = 48000.0F, .channelCount = 2},
      .maxFramesPerBuffer = 256,
  };

  std::shared_ptr<testing::NiceMock<MockAudioEventHandlerRegistry>> registry_ =
      std::make_shared<testing::NiceMock<MockAudioEventHandlerRegistry>>();
  FakeEncoderLog log_;
  PlatformFileBackend backend_ = makeFakeBackend(log_);
  std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent_ =
      std::make_shared<EventCaller<AudioEvent::RECORDER_ERROR>>(registry_);
  RecorderOutputs outputs_{errorEvent_, backend_};
};

TEST_F(RecorderOutputsTest, InvalidFilePropertiesAreRejectedAndNoFileIsWritten) {
  auto properties = wavFileProperties();
  properties->fileLayout.sampleRate = 0.0F;

  EXPECT_TRUE(outputs_.enableFile(properties).is_err());

  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  auto session = endSession();
  ASSERT_TRUE(session.is_ok());
  EXPECT_TRUE(session.unwrap().paths.empty());
}

TEST(RecorderOutputsPlatformTest, AFormatThePlatformCannotWriteIsRejected) {
  RecorderOutputs outputs(nullptr, osFileBackend());
  auto properties = wavFileProperties();
  properties->encoding.format = AudioFileProperties::FileFormat::OPUS_WEBM;

  EXPECT_TRUE(outputs.enableFile(properties).is_err());
}

TEST_F(RecorderOutputsTest, OutputsThatWereNeverRequestedStayOffAcrossSessions) {
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  EXPECT_TRUE(endSession().unwrap().paths.empty());
}

TEST_F(RecorderOutputsTest, AnOutputIsFedOnlyOnceASessionPreparesIt) {
  auto output = addFakeOutput();
  writeFrames();

  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(output->calls(), (std::vector{Call::Prepare, Call::Write, Call::Close}));
}

TEST_F(RecorderOutputsTest, AnOutputIsWrittenOffTheAudioThread) {
  auto output = addFakeOutput();
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  auto writes = output->writes();
  ASSERT_EQ(writes.size(), 1U);
  EXPECT_NE(writes[0].thread, std::this_thread::get_id());
  EXPECT_EQ(writes[0].numFrames, static_cast<size_t>(FRAMES));
}

TEST_F(RecorderOutputsTest, AnOutputAddedMidSessionIsFedAtOnce) {
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());

  auto output = addFakeOutput(MONO_48K);
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(output->calls(), (std::vector{Call::Prepare, Call::Write, Call::Close}));
}

TEST_F(RecorderOutputsTest, AnOutputThatCannotBePreparedMidSessionIsNotAdded) {
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  auto output = std::make_shared<FakeRecorderOutput>();
  output->failPrepare.store(true);

  EXPECT_TRUE(outputs_.add(output, MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());

  output->failPrepare.store(false);
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());
  EXPECT_EQ(output->calls(), std::vector{Call::Prepare});
}

TEST_F(RecorderOutputsTest, EveryFailedWriteIsReportedAndTheOutputStaysPrepared) {
  constexpr uint64_t ERROR_CALLBACK_ID = 7;
  errorEvent_->assignCallbackId(ERROR_CALLBACK_ID);
  EXPECT_CALL(*registry_, dispatchEvent(AudioEvent::RECORDER_ERROR, ERROR_CALLBACK_ID, testing::_))
      .Times(2);
  auto output = addFakeOutput();
  output->failWrite.store(true);
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());

  writeFrames();
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(output->calls(), (std::vector{Call::Prepare, Call::Write, Call::Write, Call::Close}));
}

TEST_F(RecorderOutputsTest, AnOutputCannotBeAddedTwice) {
  auto output = addFakeOutput();

  EXPECT_TRUE(outputs_.add(output, std::nullopt).is_err());
}

TEST_F(RecorderOutputsTest, AFormatChangeHandsOnQueuedFramesBeforeReopening) {
  auto output = addFakeOutput();
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  writeFrames();

  ASSERT_TRUE(outputs_.reprepare(STEREO_48K).is_ok());
  writeFrames(2);
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(
      output->calls(),
      (std::vector{
          Call::Prepare, Call::Write, Call::Close, Call::Prepare, Call::Write, Call::Close}));
  auto writes = output->writes();
  ASSERT_EQ(writes.size(), 2U);
  EXPECT_EQ(writes[0].channelCount, 1U);
  EXPECT_EQ(writes[1].channelCount, 2U);
}

TEST_F(RecorderOutputsTest, AnOutputThatCannotFollowAFormatChangeStaysOffUntilTheNextSession) {
  auto failing = addFakeOutput();
  auto following = addFakeOutput();
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  failing->failPrepare.store(true);

  EXPECT_TRUE(outputs_.reprepare(STEREO_48K).is_err());
  writeFrames(2);
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(failing->calls(), (std::vector{Call::Prepare, Call::Close, Call::Prepare}));
  EXPECT_EQ(following->writes().size(), 1U);

  failing->failPrepare.store(false);
  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());
}

TEST_F(RecorderOutputsTest, ARemovedOutputIsClosedAndNoLongerFed) {
  auto output = addFakeOutput();
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());

  outputs_.remove(output);
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(output->calls(), (std::vector{Call::Prepare, Call::Close}));
}

TEST_F(RecorderOutputsTest, AnOutputThatCannotBePreparedFailsEverySessionUntilRemoved) {
  auto output = addFakeOutput();
  output->failPrepare.store(true);

  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());
  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());

  outputs_.remove(output);
  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());
}

TEST_F(RecorderOutputsTest, DisablingTheFileClosesItMidSession) {
  ASSERT_TRUE(outputs_.enableFile(wavFileProperties()).is_ok());
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  writeFrames();
  // A buffer still queued for the effects worker when the file is disabled is not written, so
  // wait for this one to reach the encoder first.
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (log_.encodedBuffers.load() == 0 && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto disabled = RecorderOutputs::finalize(outputs_.disableFile());
  writeFrames();

  ASSERT_TRUE(disabled.is_ok());
  EXPECT_EQ(disabled.unwrap().paths.size(), 1U);
  EXPECT_EQ(log_.encodedBuffers.load(), 1);
  EXPECT_TRUE(endSession().unwrap().paths.empty());
}

TEST_F(RecorderOutputsTest, AStalledOutputNeverBlocksCaptureAndOnlyCostsTheFramesThatDidNotFit) {
  auto stalled = std::make_shared<StalledRecorderOutput>();
  ASSERT_TRUE(outputs_.add(stalled, std::nullopt).is_ok());
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  writeFrames();
  stalled->waitUntilStalled();

  // Far more buffers than the pool holds, fed while the worker is stuck in the first one.
  constexpr int kBuffersWhileStalled = 3 * static_cast<int>(kOutputsPoolSize);
  auto capture = std::async(std::launch::async, [this] {
    for (int buffer = 0; buffer < kBuffersWhileStalled; ++buffer) {
      writeFrames();
    }
  });
  const auto captureStatus = capture.wait_for(std::chrono::seconds(2));
  stalled->release();
  ASSERT_EQ(captureStatus, std::future_status::ready) << "capture waited for a stalled output";

  ASSERT_TRUE(endSession().is_ok());
  // The stalled buffer keeps its slot until its write returns, so the pool's other slots are
  // what could wait; every buffer beyond them was dropped.
  EXPECT_EQ(stalled->writes(), kOutputsPoolSize);
}

// NOLINTEND

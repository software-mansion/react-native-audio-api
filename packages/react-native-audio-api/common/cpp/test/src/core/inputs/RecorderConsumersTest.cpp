#include <audioapi/core/inputs/RecorderConsumers.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <test/src/core/inputs/FakeAudioConsumer.h>
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
constexpr size_t kOffloaderPoolSize = 32;

/// Holds the effects worker inside its first consume() until release() is called, as a slow
/// consumer would.
class StalledAudioConsumer final : public AudioConsumer {
 public:
  Result<NoneType, std::string> prepare(const StreamFormat &) override {
    return Ok(None);
  }

  Result<NoneType, std::string> consume(const AudioBuffer &, size_t) override {
    if (consumeCalls_.fetch_add(1) == 0) {
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

  [[nodiscard]] size_t consumeCalls() const {
    return consumeCalls_.load();
  }

 private:
  std::atomic<size_t> consumeCalls_{0};
  std::promise<void> entered_;
  std::promise<void> releaseSignal_;
  std::shared_future<void> released_{releaseSignal_.get_future().share()};
};

} // namespace

class RecorderConsumersTest : public ::testing::Test {
 protected:
  using Call = FakeAudioConsumer::Call;

  /// Feeds one buffer of FRAMES frames in @p channelCount channels, as the audio thread does.
  void writeFrames(int channelCount = 1) {
    const std::vector<float> samples(FRAMES, 0.25F);
    const std::vector<const float *> channels(channelCount, samples.data());
    consumers_.write(channels.data(), FRAMES);
  }

  /// Ends the session the way the recorder does, so every worker is joined.
  Result<FileInfo, std::string> endSession() {
    return RecorderConsumers::finalize(consumers_.detach());
  }

  std::shared_ptr<FakeAudioConsumer> addFakeConsumer(
      const std::optional<StreamFormat> &currentFormat = std::nullopt) {
    auto consumer = std::make_shared<FakeAudioConsumer>();
    EXPECT_TRUE(consumers_.add(consumer, currentFormat).is_ok());
    return consumer;
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
  RecorderConsumers consumers_{errorEvent_, backend_};
};

TEST_F(RecorderConsumersTest, InvalidFilePropertiesAreRejectedAndNoFileIsWritten) {
  auto properties = wavFileProperties();
  properties->fileLayout.sampleRate = 0.0F;

  EXPECT_TRUE(consumers_.enableFile(properties).is_err());

  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  auto session = endSession();
  ASSERT_TRUE(session.is_ok());
  EXPECT_TRUE(session.unwrap().paths.empty());
}

TEST(RecorderConsumersPlatformTest, AFormatThePlatformCannotWriteIsRejected) {
  RecorderConsumers consumers(nullptr, osFileBackend());
  auto properties = wavFileProperties();
  properties->encoding.format = AudioFileProperties::FileFormat::OPUS_WEBM;

  EXPECT_TRUE(consumers.enableFile(properties).is_err());
}

TEST_F(RecorderConsumersTest, AFileThatWasNeverEnabledStaysOffAcrossSessions) {
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  EXPECT_TRUE(endSession().unwrap().paths.empty());
}

TEST_F(RecorderConsumersTest, AConsumerIsFedOnlyOnceASessionPreparesIt) {
  auto consumer = addFakeConsumer();
  writeFrames();

  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(consumer->calls(), (std::vector{Call::Prepare, Call::Consume, Call::Close}));
}

TEST_F(RecorderConsumersTest, AConsumerIsFedOffTheAudioThread) {
  auto consumer = addFakeConsumer();
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  auto consumed = consumer->consumed();
  ASSERT_EQ(consumed.size(), 1U);
  EXPECT_NE(consumed[0].thread, std::this_thread::get_id());
  EXPECT_EQ(consumed[0].numFrames, static_cast<size_t>(FRAMES));
}

TEST_F(RecorderConsumersTest, AConsumerAddedMidSessionIsFedAtOnce) {
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());

  auto consumer = addFakeConsumer(MONO_48K);
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(consumer->calls(), (std::vector{Call::Prepare, Call::Consume, Call::Close}));
}

TEST_F(RecorderConsumersTest, AConsumerThatCannotBePreparedMidSessionIsNotAdded) {
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  auto consumer = std::make_shared<FakeAudioConsumer>();
  consumer->failPrepare.store(true);

  EXPECT_TRUE(consumers_.add(consumer, MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());

  consumer->failPrepare.store(false);
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());
  EXPECT_EQ(consumer->calls(), std::vector{Call::Prepare});
}

TEST_F(RecorderConsumersTest, EveryFailedConsumeIsReportedAndTheConsumerStaysPrepared) {
  constexpr uint64_t ERROR_CALLBACK_ID = 7;
  errorEvent_->assignCallbackId(ERROR_CALLBACK_ID);
  EXPECT_CALL(*registry_, dispatchEvent(AudioEvent::RECORDER_ERROR, ERROR_CALLBACK_ID, testing::_))
      .Times(2);
  auto consumer = addFakeConsumer();
  consumer->failConsume.store(true);
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());

  writeFrames();
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(
      consumer->calls(), (std::vector{Call::Prepare, Call::Consume, Call::Consume, Call::Close}));
}

TEST_F(RecorderConsumersTest, AConsumerCannotBeAddedTwice) {
  auto consumer = addFakeConsumer();

  EXPECT_TRUE(consumers_.add(consumer, std::nullopt).is_err());
}

TEST_F(RecorderConsumersTest, AFormatChangeHandsOnQueuedFramesBeforeReopening) {
  auto consumer = addFakeConsumer();
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  writeFrames();

  ASSERT_TRUE(consumers_.reprepare(STEREO_48K).is_ok());
  writeFrames(2);
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(
      consumer->calls(),
      (std::vector{
          Call::Prepare, Call::Consume, Call::Close, Call::Prepare, Call::Consume, Call::Close}));
  auto consumed = consumer->consumed();
  ASSERT_EQ(consumed.size(), 2U);
  EXPECT_EQ(consumed[0].channelCount, 1U);
  EXPECT_EQ(consumed[1].channelCount, 2U);
}

TEST_F(RecorderConsumersTest, AConsumerThatCannotFollowAFormatChangeStaysOffUntilTheNextSession) {
  auto failing = addFakeConsumer();
  auto following = addFakeConsumer();
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  failing->failPrepare.store(true);

  EXPECT_TRUE(consumers_.reprepare(STEREO_48K).is_err());
  writeFrames(2);
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(failing->calls(), (std::vector{Call::Prepare, Call::Close, Call::Prepare}));
  EXPECT_EQ(following->consumed().size(), 1U);

  failing->failPrepare.store(false);
  EXPECT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());
}

TEST_F(RecorderConsumersTest, ARemovedConsumerIsClosedAndNoLongerFed) {
  auto consumer = addFakeConsumer();
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());

  consumers_.remove(consumer);
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_EQ(consumer->calls(), (std::vector{Call::Prepare, Call::Close}));
}

TEST_F(RecorderConsumersTest, AConsumerThatCannotBePreparedFailsEverySessionUntilRemoved) {
  auto consumer = addFakeConsumer();
  consumer->failPrepare.store(true);

  EXPECT_TRUE(consumers_.prepare(MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());
  EXPECT_TRUE(consumers_.prepare(MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());

  consumers_.remove(consumer);
  EXPECT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());
}

TEST_F(RecorderConsumersTest, DisablingTheFileClosesItMidSession) {
  ASSERT_TRUE(consumers_.enableFile(wavFileProperties()).is_ok());
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  writeFrames();
  // A buffer still queued for the effects worker when the file is disabled is not written, so
  // wait for this one to reach the encoder first.
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (log_.encodedBuffers.load() == 0 && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto disabled = RecorderConsumers::finalize(consumers_.disableFile());
  writeFrames();

  ASSERT_TRUE(disabled.is_ok());
  EXPECT_EQ(disabled.unwrap().paths.size(), 1U);
  EXPECT_EQ(log_.encodedBuffers.load(), 1);
  EXPECT_TRUE(endSession().unwrap().paths.empty());
}

TEST_F(
    RecorderConsumersTest,
    AStalledConsumerNeverBlocksCaptureAndOnlyCostsTheFramesThatDidNotFit) {
  auto stalled = std::make_shared<StalledAudioConsumer>();
  ASSERT_TRUE(consumers_.add(stalled, std::nullopt).is_ok());
  ASSERT_TRUE(consumers_.prepare(MONO_48K).is_ok());
  writeFrames();
  stalled->waitUntilStalled();

  // Far more buffers than the pool holds, fed while the worker is stuck in the first one.
  constexpr int kBuffersWhileStalled = 3 * static_cast<int>(kOffloaderPoolSize);
  auto capture = std::async(std::launch::async, [this] {
    for (int buffer = 0; buffer < kBuffersWhileStalled; ++buffer) {
      writeFrames();
    }
  });
  const auto captureStatus = capture.wait_for(std::chrono::seconds(2));
  stalled->release();
  ASSERT_EQ(captureStatus, std::future_status::ready) << "capture waited for a stalled consumer";

  ASSERT_TRUE(endSession().is_ok());
  // The stalled buffer keeps its slot until its write returns, so the pool's other slots are
  // what could wait; every buffer beyond them was dropped.
  EXPECT_EQ(stalled->consumeCalls(), kOffloaderPoolSize);
}

// NOLINTEND

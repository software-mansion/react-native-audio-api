#import <AudioToolbox/AudioServices.h>
#import <XCTest/XCTest.h>

#include <mach/mach_time.h>

#import <audioapi/core/AudioPlayer.h>
#import <audioapi/core/utils/Constants.h>
#import <audioapi/ios/core/NativeAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/utils/AudioBuffer.hpp>

#include <atomic>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

using namespace audioapi;

namespace audioapi {

// Mirrors `IOSAudioPlayer.h` member for member, with the private section made protected so
// `TestableIOSAudioPlayer` can reach it. Keep the data members in the same order.
class IOSAudioPlayer : public AudioPlayer {
 public:
  IOSAudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate,
      std::atomic<uint32_t> &currentRenders,
      std::weak_ptr<AudioContext> context,
      std::mutex *driverMutex,
      AudioContextLatencyHint latencyHint);
  ~IOSAudioPlayer() override;

  bool start() override;
  void stop() override;
  bool resume() override;
  void suspend() override;
  void cleanup() override;

  [[nodiscard]] bool isRunning() const override;

  [[nodiscard]] double getBaseLatency() const override;
  [[nodiscard]] double getOutputLatency() const override;
  [[nodiscard]] std::optional<OutputPresentation> getOutputPresentation() override;
  [[nodiscard]] std::int64_t presentationClockNowNs() const override;

 protected:
  int preferredIOBufferFrames_;
  AVAudioSourceNodeRenderBlock renderBlock_;
  OnOutputRecoveryFailedBlock onStreamFail_;
  NSString *sourceNodeId_;
  std::atomic<bool> flushOverflowNextPull_;
  int pendingSavedCount_;
  std::unique_ptr<DSPAudioBuffer> pendingSaved_;
};

} // namespace audioapi

@interface FakePlayerAudioSessionManager : AudioSessionManager

@property (nonatomic, assign) BOOL ensureActiveResult;
@property (nonatomic, strong) NSError *ensureActiveFailure;
@property (nonatomic, assign) NSInteger ensureActiveCallCount;
@property (nonatomic, assign) BOOL lastEnsureActiveForce;
@property (nonatomic, assign) NSInteger fakeOutputChannelCount;

@end

@implementation FakePlayerAudioSessionManager

- (instancetype)init
{
  if (self = [super init]) {
    self.ensureActiveResult = YES;
  }

  return self;
}

- (bool)ensureActive:(bool)force error:(NSError **)error
{
  self.ensureActiveCallCount += 1;
  self.lastEnsureActiveForce = force;

  if (!self.ensureActiveResult && error != nil) {
    *error = self.ensureActiveFailure;
  } else if (error != nil) {
    *error = nil;
  }

  if (self.ensureActiveResult) {
    self.isActive = true;
  }

  return self.ensureActiveResult;
}

- (NSInteger)outputChannelCount
{
  return self.fakeOutputChannelCount;
}

@end

@interface FakePlayerAudioEngine : AudioEngine

@property (nonatomic, assign) BOOL fakeEngineRunning;
@property (nonatomic, assign) BOOL startIfNecessaryResult;
@property (nonatomic, assign) NSInteger stopIfNecessaryCallCount;
@property (nonatomic, assign) NSInteger startIfNecessaryCallCount;
@property (nonatomic, assign) NSInteger stopIfPossibleCallCount;
@property (nonatomic, assign) NSInteger attachSourceNodeCallCount;
@property (nonatomic, assign) NSInteger detachSourceNodeCallCount;
@property (nonatomic, assign) NSInteger attachInputNodeCallCount;
@property (nonatomic, copy) AVAudioSourceNodeRenderBlock lastAttachedRenderBlock;
@property (nonatomic, assign) float lastAttachedSampleRate;
@property (nonatomic, assign) AVAudioChannelCount lastAttachedChannelCount;
@property (nonatomic, copy) NSString *returnedSourceNodeId;
@property (nonatomic, copy) NSString *lastDetachedSourceNodeId;
@property (nonatomic, strong) NSMutableSet<NSString *> *attachedSourceNodeIds;
@property (nonatomic, copy) NSSet<NSString *> *sourceNodeIdsPresentAtLastStart;

@end

@implementation FakePlayerAudioEngine

- (instancetype)init
{
  if (self = [super init]) {
    self.startIfNecessaryResult = YES;
    self.returnedSourceNodeId = @"fake-source-node-id";
    self.attachedSourceNodeIds = [NSMutableSet set];
  }

  return self;
}

- (void)createAudioEngineIfNeeded
{
  // Prevent creating a real AVAudioEngine in unit tests.
}

- (bool)isEngineRunning
{
  return self.fakeEngineRunning;
}

- (void)stopIfNecessary
{
  self.stopIfNecessaryCallCount += 1;
}

- (bool)startIfNecessary:(NSError **)error
{
  self.startIfNecessaryCallCount += 1;
  self.sourceNodeIdsPresentAtLastStart = [self.attachedSourceNodeIds copy];
  return self.startIfNecessaryResult;
}

- (void)stopIfPossible
{
  self.stopIfPossibleCallCount += 1;
}

- (NSString *)attachSourceNodeWithRenderBlock:(AVAudioSourceNodeRenderBlock)renderBlock
                                   sampleRate:(float)sampleRate
                                 channelCount:(AVAudioChannelCount)channelCount
                       onOutputRecoveryFailed:(OnOutputRecoveryFailedBlock)onOutputRecoveryFailed
{
  self.attachSourceNodeCallCount += 1;
  self.lastAttachedRenderBlock = renderBlock;
  self.lastAttachedSampleRate = sampleRate;
  self.lastAttachedChannelCount = channelCount;
  [self.attachedSourceNodeIds addObject:self.returnedSourceNodeId];
  return self.returnedSourceNodeId;
}

- (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  self.detachSourceNodeCallCount += 1;
  self.lastDetachedSourceNodeId = sourceNodeId;

  if (sourceNodeId != nil) {
    [self.attachedSourceNodeIds removeObject:sourceNodeId];
  }
}

- (void)attachInputNodeWithReceiverBlock:(AVAudioSinkNodeReceiverBlock)receiverBlock
                  voiceProcessingEnabled:(BOOL)voiceProcessingEnabled
                     onInputNotification:(AudioEngineInputNotificationBlock)onInputNotification
{
  self.attachInputNodeCallCount += 1;
  (void)receiverBlock;
  (void)voiceProcessingEnabled;
  (void)onInputNotification;
}

@end

class TestableIOSAudioPlayer : public IOSAudioPlayer {
 public:
  TestableIOSAudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate)
      : currentRendersStorage_(0),
        IOSAudioPlayer(
            renderAudio,
            sampleRate,
            currentRendersStorage_,
            std::weak_ptr<AudioContext>{},
            nullptr,
            AudioContextLatencyHint::INTERACTIVE)
  {
  }

  AVAudioSourceNodeRenderBlock renderBlock() const
  {
    return renderBlock_;
  }

  NSString *sourceNodeId() const
  {
    return sourceNodeId_;
  }

  DSPAudioBuffer *getAudioBuffer() const
  {
    return renderBuffer_.get();
  }

  void setRunning(bool isRunning)
  {
    isRunning_->store(isRunning, std::memory_order_release);
  }

 private:
  std::atomic<uint32_t> currentRendersStorage_;
};

struct TestAudioOutput {
  explicit TestAudioOutput(
      UInt32 channelCount,
      AVAudioFrameCount frameCount,
      float initialValue = 0.0f)
      : storage(offsetof(AudioBufferList, mBuffers) + channelCount * sizeof(::AudioBuffer)),
        channels(channelCount, std::vector<float>(frameCount, initialValue))
  {
    AudioBufferList *audioBufferList = bufferList();
    audioBufferList->mNumberBuffers = channelCount;

    for (UInt32 channel = 0; channel < channelCount; channel += 1) {
      audioBufferList->mBuffers[channel].mNumberChannels = 1;
      audioBufferList->mBuffers[channel].mDataByteSize = frameCount * sizeof(float);
      audioBufferList->mBuffers[channel].mData = channels[channel].data();
    }
  }

  AudioBufferList *bufferList()
  {
    return reinterpret_cast<AudioBufferList *>(storage.data());
  }

  std::vector<uint8_t> storage;
  std::vector<std::vector<float>> channels;
};

static std::unique_ptr<TestableIOSAudioPlayer> makePlayer()
{
  return std::make_unique<TestableIOSAudioPlayer>([](DSPAudioBuffer *, int) {}, 48000);
}

@interface IOSAudioPlayerTests : XCTestCase

@property (nonatomic, strong) FakePlayerAudioEngine *audioEngine;
@property (nonatomic, strong) FakePlayerAudioSessionManager *sessionManager;

@end

@implementation IOSAudioPlayerTests

- (void)setUp
{
  [super setUp];

  self.audioEngine = [[FakePlayerAudioEngine alloc] init];
  self.sessionManager = [[FakePlayerAudioSessionManager alloc] init];
  self.audioEngine.sessionManager = self.sessionManager;
}

- (void)tearDown
{
  [self.audioEngine cleanup];
  [self.sessionManager cleanup];
  self.audioEngine = nil;
  self.sessionManager = nil;

  [super tearDown];
}

- (void)resetEngineCallCounts
{
  self.audioEngine.stopIfNecessaryCallCount = 0;
  self.audioEngine.attachSourceNodeCallCount = 0;
  self.audioEngine.startIfNecessaryCallCount = 0;
  self.audioEngine.detachSourceNodeCallCount = 0;
  self.audioEngine.stopIfPossibleCallCount = 0;
  self.audioEngine.lastDetachedSourceNodeId = nil;
  self.sessionManager.ensureActiveCallCount = 0;
}

- (void)testRenderBlockReturnsErrorWhenBufferCountDoesNotMatch
{
  int renderCallCount = 0;
  auto player = std::make_unique<TestableIOSAudioPlayer>(
      [&renderCallCount](DSPAudioBuffer *, int) { renderCallCount += 1; }, 48000);
  player->setRunning(true);
  TestAudioOutput output(1, 64, 1.0f);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};

  OSStatus status = player->renderBlock()(&isSilence, &timestamp, 64, output.bufferList());

  XCTAssertEqual(status, kAudioServicesBadPropertySizeError);
  XCTAssertEqual(renderCallCount, 0);
}

- (void)testStartAndResumeActivateSessionAttachSourceAndStartEngine
{
  auto assertStartLikeOperation = [&](bool useResume) {
    auto player = makePlayer();

    XCTAssertTrue(useResume ? player->resume() : player->start());
    XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 1);
    XCTAssertFalse(self.sessionManager.lastEnsureActiveForce);
    XCTAssertEqual(self.audioEngine.stopIfNecessaryCallCount, 1);
    XCTAssertEqual(self.audioEngine.attachSourceNodeCallCount, 1);
    XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 1);
    XCTAssertNotNil(self.audioEngine.lastAttachedRenderBlock);
    XCTAssertEqualWithAccuracy(self.audioEngine.lastAttachedSampleRate, 48000.0f, 0.001f);
    XCTAssertEqual(self.audioEngine.lastAttachedChannelCount, 2U);
    XCTAssertEqualObjects(player->sourceNodeId(), self.audioEngine.returnedSourceNodeId);

    self.audioEngine.fakeEngineRunning = YES;
    self.audioEngine.state = AudioEngineStateRunning;
    XCTAssertTrue(player->isRunning());

    self.audioEngine.fakeEngineRunning = NO;
    [self resetEngineCallCounts];
  };

  assertStartLikeOperation(false);
  assertStartLikeOperation(true);
}

- (void)testStartAttachesWithTheRouteOutputChannelCount
{
  auto player = makePlayer();
  self.sessionManager.fakeOutputChannelCount = 6;

  XCTAssertTrue(player->start());

  XCTAssertEqual(self.audioEngine.lastAttachedChannelCount, 6U);
  XCTAssertEqual(player->getAudioBuffer()->getNumberOfChannels(), 6UL);
}

- (void)testStartFallsBackToStereoWhenTheRouteReportsNoChannels
{
  auto player = makePlayer();
  self.sessionManager.fakeOutputChannelCount = 0;

  XCTAssertTrue(player->start());

  XCTAssertEqual(self.audioEngine.lastAttachedChannelCount, 2U);
  XCTAssertEqual(player->getAudioBuffer()->getNumberOfChannels(), 2UL);
}

- (void)testRestartAfterRouteChangeRendersTheNewChannelCount
{
  auto player = makePlayer();
  self.sessionManager.fakeOutputChannelCount = 1;
  XCTAssertTrue(player->start());
  player->stop();

  self.sessionManager.fakeOutputChannelCount = 8;
  XCTAssertTrue(player->start());

  XCTAssertEqual(self.audioEngine.lastAttachedChannelCount, 8U);
  TestAudioOutput output(8, 128, -1.0f);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};
  XCTAssertEqual(
      player->renderBlock()(&isSilence, &timestamp, 128, output.bufferList()),
      kAudioServicesNoError);
}

- (void)testStartReturnsFalseWhenSessionActivationFails
{
  auto player = makePlayer();
  self.sessionManager.ensureActiveResult = NO;
  self.sessionManager.ensureActiveFailure = [NSError errorWithDomain:@"AudioPlayerTests"
                                                                code:1
                                                            userInfo:nil];

  XCTAssertFalse(player->start());
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 1);
  XCTAssertEqual(self.audioEngine.stopIfNecessaryCallCount, 0);
  XCTAssertEqual(self.audioEngine.attachSourceNodeCallCount, 0);
  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 0);
  XCTAssertNil(player->sourceNodeId());
}

- (void)testStartAndResumeDetachSourceWhenEngineFailsToStart
{
  auto assertFailedStart = [&](bool useResume) {
    auto player = makePlayer();
    self.audioEngine.startIfNecessaryResult = NO;

    XCTAssertFalse(useResume ? player->resume() : player->start());
    XCTAssertEqual(self.audioEngine.stopIfNecessaryCallCount, 1);
    XCTAssertEqual(self.audioEngine.attachSourceNodeCallCount, 1);
    XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 1);
    XCTAssertEqual(self.audioEngine.detachSourceNodeCallCount, 1);
    XCTAssertEqualObjects(
        self.audioEngine.lastDetachedSourceNodeId, self.audioEngine.returnedSourceNodeId);
    XCTAssertEqual(self.audioEngine.stopIfPossibleCallCount, 1);
    XCTAssertNil(player->sourceNodeId());
    XCTAssertEqual(self.audioEngine.attachedSourceNodeIds.count, 0UL);

    player.reset();
    [self resetEngineCallCounts];
  };

  assertFailedStart(false);
  assertFailedStart(true);
}

- (void)testFailedPlayerStartLeavesNoSourceForFollowingRecorderStart
{
  auto player = makePlayer();
  self.audioEngine.startIfNecessaryResult = NO;

  XCTAssertFalse(player->start());
  XCTAssertEqual(self.audioEngine.attachedSourceNodeIds.count, 0UL);
  XCTAssertNil(player->sourceNodeId());

  self.audioEngine.startIfNecessaryResult = YES;
  NativeAudioRecorder *recorder = [[NativeAudioRecorder alloc]
       initWithReceiverBlock:^(const AudioBufferList *inputBuffer, int numFrames) {}
      voiceProcessingEnabled:NO];

  XCTAssertTrue([recorder start:nil]);
  XCTAssertEqual(self.audioEngine.attachInputNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.attachedSourceNodeIds.count, 0UL);
  XCTAssertEqual(self.audioEngine.sourceNodeIdsPresentAtLastStart.count, 0UL);
}

- (void)testStartDoesNotReattachAnAttachedSource
{
  auto player = makePlayer();

  // The engine is not reported running, so the second start goes past the running check.
  XCTAssertTrue(player->start());
  XCTAssertTrue(player->start());

  XCTAssertEqual(self.audioEngine.attachSourceNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 2);
  XCTAssertEqualObjects(player->sourceNodeId(), self.audioEngine.returnedSourceNodeId);
}

- (void)testStartShortCircuitsWhenAlreadyRunning
{
  auto player = makePlayer();
  player->setRunning(true);
  self.audioEngine.fakeEngineRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  XCTAssertTrue(player->start());
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 0);
  XCTAssertEqual(self.audioEngine.attachSourceNodeCallCount, 0);
}

- (void)testStopAndSuspendDetachSourceAndClearRunningState
{
  auto assertStopLikeOperation = [&](bool useSuspend) {
    auto player = makePlayer();
    XCTAssertTrue(player->start());
    self.audioEngine.fakeEngineRunning = YES;
    self.audioEngine.state = AudioEngineStateRunning;

    if (useSuspend) {
      player->suspend();
    } else {
      player->stop();
    }

    XCTAssertEqual(self.audioEngine.detachSourceNodeCallCount, 1);
    XCTAssertEqualObjects(
        self.audioEngine.lastDetachedSourceNodeId, self.audioEngine.returnedSourceNodeId);
    XCTAssertEqual(self.audioEngine.stopIfPossibleCallCount, 1);
    XCTAssertNil(player->sourceNodeId());
    XCTAssertFalse(player->isRunning());

    player.reset();
    self.audioEngine.fakeEngineRunning = NO;
    [self resetEngineCallCounts];
  };

  assertStopLikeOperation(false);
  assertStopLikeOperation(true);
}

- (void)testStopWithoutAttachedSourceDoesNotDetach
{
  auto player = makePlayer();

  player->stop();

  XCTAssertEqual(self.audioEngine.detachSourceNodeCallCount, 0);
  XCTAssertEqual(self.audioEngine.stopIfPossibleCallCount, 1);
  XCTAssertNil(player->sourceNodeId());
}

- (void)testCleanupDetachesSourceAndClearsAudioBuffer
{
  auto player = makePlayer();
  XCTAssertTrue(player->start());

  player->cleanup();

  XCTAssertEqual(self.audioEngine.detachSourceNodeCallCount, 1);
  XCTAssertNil(player->sourceNodeId());
  XCTAssertEqual(player->getAudioBuffer(), nullptr);
}

// The engine keeps the render block until the node is detached and may still call it after the
// player is gone.
- (void)testRenderBlockOutputsSilenceOnceThePlayerIsGone
{
  int renderCallCount = 0;
  auto player = std::make_unique<TestableIOSAudioPlayer>(
      [&renderCallCount](DSPAudioBuffer *, int) { renderCallCount += 1; }, 48000);
  player->setRunning(true);
  AVAudioSourceNodeRenderBlock renderBlock = player->renderBlock();
  player.reset();

  TestAudioOutput output(2, 64, 5.0f);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};

  OSStatus status = renderBlock(&isSilence, &timestamp, 64, output.bufferList());

  XCTAssertEqual(status, kAudioServicesNoError);
  XCTAssertEqual(renderCallCount, 0);
  for (int channel = 0; channel < 2; channel += 1) {
    for (int frame = 0; frame < 64; frame += 1) {
      XCTAssertEqualWithAccuracy(output.channels[channel][frame], 0.0f, 0.0001f);
    }
  }
}

- (void)testIsRunningRequiresInternalFlagEngineAndRunningState
{
  auto player = std::make_unique<TestableIOSAudioPlayer>([](DSPAudioBuffer *, int) {}, 48000);

  player->setRunning(false);
  self.audioEngine.fakeEngineRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;
  XCTAssertFalse(player->isRunning());

  player->setRunning(true);
  self.audioEngine.fakeEngineRunning = NO;
  XCTAssertFalse(player->isRunning());

  self.audioEngine.fakeEngineRunning = YES;
  self.audioEngine.state = AudioEngineStatePaused;
  XCTAssertFalse(player->isRunning());

  self.audioEngine.state = AudioEngineStateRunning;
  XCTAssertTrue(player->isRunning());
}

- (void)testRenderBlockRendersInQuantumSizedChunksWhenRunning
{
  std::vector<int> renderedFrameSizes;
  float delta = 0.1f;
  auto player = std::make_unique<TestableIOSAudioPlayer>(
      [&renderedFrameSizes, &delta](DSPAudioBuffer *buffer, int numFrames) {
        renderedFrameSizes.push_back(numFrames);

        for (int channel = 0; channel < 2; channel += 1) {
          for (int frame = 0; frame < numFrames; frame += 1) {
            (*buffer)[channel][frame] = (channel == 0 ? 0.1f : 0.2f) + delta;
          }
        }

        delta += 0.1f;
      },
      48000);

  player->setRunning(true);
  TestAudioOutput output(2, 300, -1.0f);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};

  OSStatus status = player->renderBlock()(&isSilence, &timestamp, 300, output.bufferList());

  XCTAssertEqual(status, kAudioServicesNoError);
  XCTAssertEqual(renderedFrameSizes.size(), 3UL);
  XCTAssertEqual(renderedFrameSizes[0], RENDER_QUANTUM_SIZE);
  XCTAssertEqual(renderedFrameSizes[1], RENDER_QUANTUM_SIZE);
  XCTAssertEqual(renderedFrameSizes[2], RENDER_QUANTUM_SIZE);

  for (int frame = 0; frame < 128; frame += 1) {
    XCTAssertEqualWithAccuracy(output.channels[0][frame], 0.2f, 0.0001f);
    XCTAssertEqualWithAccuracy(output.channels[1][frame], 0.3f, 0.0001f);
  }

  for (int frame = 128; frame < 256; frame += 1) {
    XCTAssertEqualWithAccuracy(output.channels[0][frame], 0.3f, 0.0001f);
    XCTAssertEqualWithAccuracy(output.channels[1][frame], 0.4f, 0.0001f);
  }

  for (int frame = 256; frame < 300; frame += 1) {
    XCTAssertEqualWithAccuracy(output.channels[0][frame], 0.4f, 0.0001f);
    XCTAssertEqualWithAccuracy(output.channels[1][frame], 0.5f, 0.0001f);
  }
}

- (void)testOutputPresentationIsEmptyWithoutAValidHostTime
{
  auto player = std::make_unique<TestableIOSAudioPlayer>([](DSPAudioBuffer *, int) {}, 48000);
  player->setRunning(true);
  TestAudioOutput output(2, 300);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};

  XCTAssertFalse(player->getOutputPresentation().has_value());

  player->renderBlock()(&isSilence, &timestamp, 300, output.bufferList());

  XCTAssertFalse(player->getOutputPresentation().has_value());
}

- (void)testOutputPresentationStartsAtTheBufferFirstFrameAcrossOverflow
{
  auto player = std::make_unique<TestableIOSAudioPlayer>([](DSPAudioBuffer *, int) {}, 48000);
  player->setRunning(true);
  BOOL isSilence = NO;

  // 300-frame buffers pull whole quanta, so each buffer starts with the previous one's leftover.
  const std::int64_t expectedFirstFrames[] = {0, 300, 600};
  std::int64_t previousPresentationTimeNs = 0;
  for (const std::int64_t expectedFirstFrame : expectedFirstFrames) {
    TestAudioOutput output(2, 300);
    AudioTimeStamp timestamp = {};
    timestamp.mFlags = kAudioTimeStampHostTimeValid;
    timestamp.mHostTime = mach_absolute_time();

    player->renderBlock()(&isSilence, &timestamp, 300, output.bufferList());

    const auto presentation = player->getOutputPresentation();
    XCTAssertTrue(presentation.has_value());
    XCTAssertEqual(presentation->contextFrame, expectedFirstFrame);
    XCTAssertGreaterThanOrEqual(presentation->presentationTimeNs, previousPresentationTimeNs);
    previousPresentationTimeNs = presentation->presentationTimeNs;
  }
}

// `performance.now()` reads mach uptime on iOS. A time moved onto `steady_clock` is ahead of it by
// however long the machine has slept since boot.
- (void)testOutputPresentationStaysOnTheCallbackHostClock
{
  auto player = std::make_unique<TestableIOSAudioPlayer>([](DSPAudioBuffer *, int) {}, 48000);
  player->setRunning(true);
  TestAudioOutput output(2, 128);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};
  timestamp.mFlags = kAudioTimeStampHostTimeValid;
  timestamp.mHostTime = mach_absolute_time();

  player->renderBlock()(&isSilence, &timestamp, 128, output.bufferList());

  mach_timebase_info_data_t timebase{};
  mach_timebase_info(&timebase);
  const auto hostTimeNs = static_cast<std::int64_t>(
      static_cast<__int128>(timestamp.mHostTime) * timebase.numer / timebase.denom);
  const auto outputLatencyNs = static_cast<std::int64_t>(
      std::llround([[AudioSessionManager sharedInstance] outputLatencySeconds] * 1e9));
  const auto presentation = player->getOutputPresentation();
  XCTAssertTrue(presentation.has_value());
  XCTAssertEqual(presentation->presentationTimeNs, hostTimeNs + outputLatencyNs);
}

- (void)testPresentationClockIsMachUptime
{
  auto player = std::make_unique<TestableIOSAudioPlayer>([](DSPAudioBuffer *, int) {}, 48000);

  const auto before = static_cast<std::int64_t>(clock_gettime_nsec_np(CLOCK_UPTIME_RAW));
  const auto now = player->presentationClockNowNs();
  const auto after = static_cast<std::int64_t>(clock_gettime_nsec_np(CLOCK_UPTIME_RAW));

  XCTAssertGreaterThanOrEqual(now, before);
  XCTAssertLessThanOrEqual(now, after);
}

- (void)testRenderBlockZerosBufferedAudioWhenPlayerIsNotRunning
{
  int renderCallCount = 0;
  auto player = std::make_unique<TestableIOSAudioPlayer>(
      [&renderCallCount](DSPAudioBuffer *, int) { renderCallCount += 1; }, 48000);

  player->setRunning(false);
  for (int channel = 0; channel < 2; channel += 1) {
    for (int frame = 0; frame < static_cast<int>(player->getAudioBuffer()->getSize()); frame += 1) {
      (*player->getAudioBuffer())[channel][frame] = 0.75f;
    }
  }

  TestAudioOutput output(2, 64, 5.0f);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};

  OSStatus status = player->renderBlock()(&isSilence, &timestamp, 64, output.bufferList());

  XCTAssertEqual(status, kAudioServicesNoError);
  XCTAssertEqual(renderCallCount, 0);

  for (int channel = 0; channel < 2; channel += 1) {
    for (int frame = 0; frame < 64; frame += 1) {
      XCTAssertEqualWithAccuracy(output.channels[channel][frame], 0.0f, 0.0001f);
    }
  }
}

@end

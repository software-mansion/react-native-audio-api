#import <AudioToolbox/AudioServices.h>
#import <XCTest/XCTest.h>

#import <audioapi/core/utils/Constants.h>
#import <audioapi/ios/core/NativeAudioPlayer.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/utils/AudioBuffer.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using namespace audioapi;

namespace audioapi {

class IOSAudioPlayer {
 public:
  IOSAudioPlayer(
      const std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> &renderAudio,
      float sampleRate,
      int channelCount);
  ~IOSAudioPlayer();

  bool start();
  void stop();
  bool resume();
  void suspend();
  void cleanup();

  bool isRunning() const;

 protected:
  std::shared_ptr<DSPAudioBuffer> audioBuffer_;
  NativeAudioPlayer *audioPlayer_;
  std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> renderAudio_;
  int channelCount_;
  std::atomic<bool> isRunning_;
};

} // namespace audioapi

@interface NativeAudioPlayer (TestingPrivate)

- (void)attachSourceNodeIfNeeded:(AudioEngine *)audioEngine;
- (void)detachSourceNodeIfAttached:(AudioEngine *)audioEngine;

@end

@interface FakePlayerAudioSessionManager : AudioSessionManager

@property(nonatomic, assign) BOOL ensureActiveResult;
@property(nonatomic, strong) NSError *ensureActiveFailure;
@property(nonatomic, assign) NSInteger ensureActiveCallCount;
@property(nonatomic, assign) BOOL lastEnsureActiveForce;

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

@end

@interface FakePlayerAudioEngine : AudioEngine

@property(nonatomic, assign) BOOL fakeEngineRunning;
@property(nonatomic, assign) BOOL startIfNecessaryResult;
@property(nonatomic, assign) NSInteger stopIfNecessaryCallCount;
@property(nonatomic, assign) NSInteger startIfNecessaryCallCount;
@property(nonatomic, assign) NSInteger stopIfPossibleCallCount;
@property(nonatomic, assign) NSInteger attachSourceNodeCallCount;
@property(nonatomic, assign) NSInteger detachSourceNodeCallCount;
@property(nonatomic, copy) AVAudioSourceNodeRenderBlock lastAttachedRenderBlock;
@property(nonatomic, assign) float lastAttachedSampleRate;
@property(nonatomic, assign) AVAudioChannelCount lastAttachedChannelCount;
@property(nonatomic, copy) NSString *returnedSourceNodeId;
@property(nonatomic, copy) NSString *lastDetachedSourceNodeId;

@end

@implementation FakePlayerAudioEngine

- (instancetype)init
{
  if (self = [super init]) {
    self.startIfNecessaryResult = YES;
    self.returnedSourceNodeId = @"fake-source-node-id";
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

- (bool)startIfNecessary
{
  self.startIfNecessaryCallCount += 1;
  return self.startIfNecessaryResult;
}

- (void)stopIfPossible
{
  self.stopIfPossibleCallCount += 1;
}

- (NSString *)attachSourceNodeWithRenderBlock:(AVAudioSourceNodeRenderBlock)renderBlock
                                   sampleRate:(float)sampleRate
                                 channelCount:(AVAudioChannelCount)channelCount
{
  self.attachSourceNodeCallCount += 1;
  self.lastAttachedRenderBlock = renderBlock;
  self.lastAttachedSampleRate = sampleRate;
  self.lastAttachedChannelCount = channelCount;
  return self.returnedSourceNodeId;
}

- (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  self.detachSourceNodeCallCount += 1;
  self.lastDetachedSourceNodeId = sourceNodeId;
}

@end

@interface FakeNativeAudioPlayer : NativeAudioPlayer

@property(nonatomic, assign) BOOL startResult;
@property(nonatomic, assign) BOOL resumeResult;
@property(nonatomic, assign) NSInteger startCallCount;
@property(nonatomic, assign) NSInteger stopCallCount;
@property(nonatomic, assign) NSInteger resumeCallCount;
@property(nonatomic, assign) NSInteger suspendCallCount;
@property(nonatomic, assign) NSInteger cleanupCallCount;

@end

@implementation FakeNativeAudioPlayer

- (instancetype)init
{
  if (self = [super init]) {
    self.startResult = YES;
    self.resumeResult = YES;
  }

  return self;
}

- (bool)start
{
  self.startCallCount += 1;
  return self.startResult;
}

- (void)stop
{
  self.stopCallCount += 1;
}

- (bool)resume
{
  self.resumeCallCount += 1;
  return self.resumeResult;
}

- (void)suspend
{
  self.suspendCallCount += 1;
}

- (void)cleanup
{
  self.cleanupCallCount += 1;
}

@end

class TestableIOSAudioPlayer : public IOSAudioPlayer {
 public:
  TestableIOSAudioPlayer(
      const std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> &renderAudio,
      float sampleRate,
      int channelCount)
      : IOSAudioPlayer(renderAudio, sampleRate, channelCount) {}

  NativeAudioPlayer *replaceAudioPlayer(NativeAudioPlayer *audioPlayer) {
    NativeAudioPlayer *previous = audioPlayer_;
    audioPlayer_ = audioPlayer;
    return previous;
  }

  NativeAudioPlayer *getAudioPlayer() const {
    return audioPlayer_;
  }

  std::shared_ptr<DSPAudioBuffer> getAudioBuffer() const {
    return audioBuffer_;
  }

  void setRunning(bool isRunning) {
    isRunning_.store(isRunning, std::memory_order_release);
  }
};

struct TestAudioOutput {
  explicit TestAudioOutput(UInt32 channelCount, AVAudioFrameCount frameCount, float initialValue = 0.0f)
      : storage(offsetof(AudioBufferList, mBuffers) + channelCount * sizeof(::AudioBuffer)),
        channels(channelCount, std::vector<float>(frameCount, initialValue)) {
    AudioBufferList *audioBufferList = bufferList();
    audioBufferList->mNumberBuffers = channelCount;

    for (UInt32 channel = 0; channel < channelCount; channel += 1) {
      audioBufferList->mBuffers[channel].mNumberChannels = 1;
      audioBufferList->mBuffers[channel].mDataByteSize = frameCount * sizeof(float);
      audioBufferList->mBuffers[channel].mData = channels[channel].data();
    }
  }

  AudioBufferList *bufferList() {
    return reinterpret_cast<AudioBufferList *>(storage.data());
  }

  std::vector<uint8_t> storage;
  std::vector<std::vector<float>> channels;
};

@interface NativeAudioPlayerTests : XCTestCase

@property(nonatomic, strong) FakePlayerAudioEngine *audioEngine;
@property(nonatomic, strong) FakePlayerAudioSessionManager *sessionManager;

@end

@implementation NativeAudioPlayerTests

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

- (NativeAudioPlayer *)createPlayerWithRenderCallCount:(NSInteger *)renderCallCount
{
  return [[NativeAudioPlayer alloc] initWithRenderAudio:^(AudioBufferList *outputBuffer, int numFrames) {
    if (renderCallCount != nullptr) {
      *renderCallCount += 1;
    }
  }
                                             sampleRate:48000
                                           channelCount:2];
}

- (void)assertStartLikeOperationRunsGraphForSelector:(SEL)selector
{
  NativeAudioPlayer *player = [self createPlayerWithRenderCallCount:nullptr];
  typedef BOOL (*NativeAudioPlayerBoolMethod)(id, SEL);
  NativeAudioPlayerBoolMethod operation =
      (NativeAudioPlayerBoolMethod)[player methodForSelector:selector];

  XCTAssertTrue(operation(player, selector));
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 1);
  XCTAssertFalse(self.sessionManager.lastEnsureActiveForce);
  XCTAssertEqual(self.audioEngine.stopIfNecessaryCallCount, 1);
  XCTAssertEqual(self.audioEngine.attachSourceNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 1);
  XCTAssertNotNil(self.audioEngine.lastAttachedRenderBlock);
  XCTAssertEqualWithAccuracy(self.audioEngine.lastAttachedSampleRate, 48000.0f, 0.001f);
  XCTAssertEqual(self.audioEngine.lastAttachedChannelCount, 2U);
  XCTAssertEqualObjects(player.sourceNodeId, self.audioEngine.returnedSourceNodeId);
}

- (void)assertStopLikeOperationDetachesSourceForSelector:(SEL)selector
{
  NativeAudioPlayer *player = [self createPlayerWithRenderCallCount:nullptr];
  player.sourceNodeId = @"attached-source-node";
  typedef void (*NativeAudioPlayerVoidMethod)(id, SEL);
  NativeAudioPlayerVoidMethod operation =
      (NativeAudioPlayerVoidMethod)[player methodForSelector:selector];

  operation(player, selector);

  XCTAssertEqual(self.audioEngine.detachSourceNodeCallCount, 1);
  XCTAssertEqualObjects(self.audioEngine.lastDetachedSourceNodeId, @"attached-source-node");
  XCTAssertEqual(self.audioEngine.stopIfPossibleCallCount, 1);
  XCTAssertNil(player.sourceNodeId);
}

- (void)testRenderBlockReturnsErrorWhenBufferCountDoesNotMatch
{
  NSInteger renderCallCount = 0;
  NativeAudioPlayer *player = [self createPlayerWithRenderCallCount:&renderCallCount];
  TestAudioOutput output(1, 64, 1.0f);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};

  OSStatus status = player.renderBlock(&isSilence, &timestamp, 64, output.bufferList());

  XCTAssertEqual(status, kAudioServicesBadPropertySizeError);
  XCTAssertEqual(renderCallCount, 0);
}

- (void)testRenderBlockInvokesRenderAudioWhenBufferCountMatches
{
  __block NSInteger renderCallCount = 0;
  __block AVAudioFrameCount renderedFrameCount = 0;
  __block AudioBufferList *renderedBufferList = nullptr;
  NativeAudioPlayer *player = [[NativeAudioPlayer alloc]
      initWithRenderAudio:^(AudioBufferList *outputBuffer, int numFrames) {
        renderCallCount += 1;
        renderedFrameCount = static_cast<AVAudioFrameCount>(numFrames);
        renderedBufferList = outputBuffer;
      }
            sampleRate:48000
          channelCount:2];
  TestAudioOutput output(2, 64, 0.0f);
  BOOL isSilence = NO;
  AudioTimeStamp timestamp = {};

  OSStatus status = player.renderBlock(&isSilence, &timestamp, 64, output.bufferList());

  XCTAssertEqual(status, kAudioServicesNoError);
  XCTAssertEqual(renderCallCount, 1);
  XCTAssertEqual(renderedFrameCount, 64U);
  XCTAssertEqual(renderedBufferList, output.bufferList());
}

- (void)testStartAndResumeActivateSessionAttachSourceAndStartEngine
{
  [self assertStartLikeOperationRunsGraphForSelector:@selector(start)];

  self.audioEngine.stopIfNecessaryCallCount = 0;
  self.audioEngine.attachSourceNodeCallCount = 0;
  self.audioEngine.startIfNecessaryCallCount = 0;
  self.sessionManager.ensureActiveCallCount = 0;

  [self assertStartLikeOperationRunsGraphForSelector:@selector(resume)];
}

- (void)testStartReturnsFalseWhenSessionActivationFails
{
  NativeAudioPlayer *player = [self createPlayerWithRenderCallCount:nullptr];
  self.sessionManager.ensureActiveResult = NO;
  self.sessionManager.ensureActiveFailure =
      [NSError errorWithDomain:@"AudioPlayerTests" code:1 userInfo:nil];

  XCTAssertFalse([player start]);
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 1);
  XCTAssertEqual(self.audioEngine.stopIfNecessaryCallCount, 0);
  XCTAssertEqual(self.audioEngine.attachSourceNodeCallCount, 0);
  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 0);
  XCTAssertNil(player.sourceNodeId);
}

- (void)testAttachSourceNodeIfNeededIsIdempotent
{
  NativeAudioPlayer *player = [self createPlayerWithRenderCallCount:nullptr];
  player.sourceNodeId = @"existing-source-node";

  [player attachSourceNodeIfNeeded:self.audioEngine];

  XCTAssertEqual(self.audioEngine.attachSourceNodeCallCount, 0);
  XCTAssertEqualObjects(player.sourceNodeId, @"existing-source-node");
}

- (void)testStopAndSuspendDetachAttachedSourceAndStopIfPossible
{
  [self assertStopLikeOperationDetachesSourceForSelector:@selector(stop)];

  self.audioEngine.detachSourceNodeCallCount = 0;
  self.audioEngine.stopIfPossibleCallCount = 0;
  self.audioEngine.lastDetachedSourceNodeId = nil;

  [self assertStopLikeOperationDetachesSourceForSelector:@selector(suspend)];
}

- (void)testDetachSourceNodeIfAttachedNoOpsWhenNotAttached
{
  NativeAudioPlayer *player = [self createPlayerWithRenderCallCount:nullptr];

  [player detachSourceNodeIfAttached:self.audioEngine];

  XCTAssertEqual(self.audioEngine.detachSourceNodeCallCount, 0);
  XCTAssertNil(player.sourceNodeId);
}

@end

@interface IOSAudioPlayerTests : XCTestCase

@property(nonatomic, strong) FakePlayerAudioEngine *audioEngine;
@property(nonatomic, strong) FakePlayerAudioSessionManager *sessionManager;

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

- (void)testStartAndResumeReturnNativeResultAndUpdateRunningState
{
  auto assertOperationTracksRunning = [&](bool useResume) {
    auto player = std::make_unique<TestableIOSAudioPlayer>(
        [](std::shared_ptr<DSPAudioBuffer>, int) {}, 48000, 2);
    FakeNativeAudioPlayer *fakeNative = [[FakeNativeAudioPlayer alloc] init];
    NativeAudioPlayer *originalNative = player->replaceAudioPlayer(fakeNative);
    [originalNative cleanup];

    if (useResume) {
      XCTAssertTrue(player->resume());
      XCTAssertEqual(fakeNative.resumeCallCount, 1);
    } else {
      XCTAssertTrue(player->start());
      XCTAssertEqual(fakeNative.startCallCount, 1);
    }

    self.audioEngine.fakeEngineRunning = YES;
    self.audioEngine.state = AudioEngineStateRunning;
    XCTAssertTrue(player->isRunning());
  };

  assertOperationTracksRunning(false);
  assertOperationTracksRunning(true);
}

- (void)testStartShortCircuitsWhenAlreadyRunning
{
  auto player =
      std::make_unique<TestableIOSAudioPlayer>([](std::shared_ptr<DSPAudioBuffer>, int) {}, 48000, 2);
  FakeNativeAudioPlayer *fakeNative = [[FakeNativeAudioPlayer alloc] init];
  NativeAudioPlayer *originalNative = player->replaceAudioPlayer(fakeNative);
  [originalNative cleanup];

  player->setRunning(true);
  self.audioEngine.fakeEngineRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  XCTAssertTrue(player->start());
  XCTAssertEqual(fakeNative.startCallCount, 0);
}

- (void)testStopAndSuspendClearRunningStateAndForwardToNativePlayer
{
  auto assertOperationStopsRunning = [&](bool useSuspend) {
    auto player = std::make_unique<TestableIOSAudioPlayer>(
        [](std::shared_ptr<DSPAudioBuffer>, int) {}, 48000, 2);
    FakeNativeAudioPlayer *fakeNative = [[FakeNativeAudioPlayer alloc] init];
    NativeAudioPlayer *originalNative = player->replaceAudioPlayer(fakeNative);
    [originalNative cleanup];

    player->setRunning(true);
    self.audioEngine.fakeEngineRunning = YES;
    self.audioEngine.state = AudioEngineStateRunning;

    if (useSuspend) {
      player->suspend();
      XCTAssertEqual(fakeNative.suspendCallCount, 1);
    } else {
      player->stop();
      XCTAssertEqual(fakeNative.stopCallCount, 1);
    }

    XCTAssertFalse(player->isRunning());
  };

  assertOperationStopsRunning(false);
  assertOperationStopsRunning(true);
}

- (void)testCleanupStopsThenCleansUpAndClearsAudioBuffer
{
  auto player =
      std::make_unique<TestableIOSAudioPlayer>([](std::shared_ptr<DSPAudioBuffer>, int) {}, 48000, 2);
  FakeNativeAudioPlayer *fakeNative = [[FakeNativeAudioPlayer alloc] init];
  NativeAudioPlayer *originalNative = player->replaceAudioPlayer(fakeNative);
  [originalNative cleanup];

  player->setRunning(true);
  player->cleanup();

  XCTAssertEqual(fakeNative.stopCallCount, 1);
  XCTAssertEqual(fakeNative.cleanupCallCount, 1);
  XCTAssertEqual(player->getAudioBuffer(), nullptr);
}

- (void)testIsRunningRequiresInternalFlagEngineAndRunningState
{
  auto player =
      std::make_unique<TestableIOSAudioPlayer>([](std::shared_ptr<DSPAudioBuffer>, int) {}, 48000, 2);

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
  int chunkIndex = 0;
  auto player = std::make_unique<TestableIOSAudioPlayer>(
      [&renderedFrameSizes, &chunkIndex](std::shared_ptr<DSPAudioBuffer> buffer, int numFrames) {
        renderedFrameSizes.push_back(numFrames);

        for (int channel = 0; channel < 2; channel += 1) {
          for (int frame = 0; frame < numFrames; frame += 1) {
            (*buffer->getChannel(channel))[frame] = (channel == 0 ? 1.0f : 10.0f) * (chunkIndex + 1);
          }
        }

        chunkIndex += 1;
      },
      48000,
      2);

  player->setRunning(true);
  TestAudioOutput output(2, 300, -1.0f);
  BOOL isSilence = NO;
  NativeAudioPlayer *nativePlayer = player->getAudioPlayer();
  AudioTimeStamp timestamp = {};

  OSStatus status = nativePlayer.renderBlock(&isSilence, &timestamp, 300, output.bufferList());

  XCTAssertEqual(status, kAudioServicesNoError);
  XCTAssertEqual(renderedFrameSizes.size(), 3UL);
  XCTAssertEqual(renderedFrameSizes[0], RENDER_QUANTUM_SIZE);
  XCTAssertEqual(renderedFrameSizes[1], RENDER_QUANTUM_SIZE);
  XCTAssertEqual(renderedFrameSizes[2], 44);

  for (int frame = 0; frame < 128; frame += 1) {
    XCTAssertEqualWithAccuracy(output.channels[0][frame], 1.0f, 0.0001f);
    XCTAssertEqualWithAccuracy(output.channels[1][frame], 10.0f, 0.0001f);
  }

  for (int frame = 128; frame < 256; frame += 1) {
    XCTAssertEqualWithAccuracy(output.channels[0][frame], 2.0f, 0.0001f);
    XCTAssertEqualWithAccuracy(output.channels[1][frame], 20.0f, 0.0001f);
  }

  for (int frame = 256; frame < 300; frame += 1) {
    XCTAssertEqualWithAccuracy(output.channels[0][frame], 3.0f, 0.0001f);
    XCTAssertEqualWithAccuracy(output.channels[1][frame], 30.0f, 0.0001f);
  }
}

- (void)testRenderBlockZerosBufferedAudioWhenPlayerIsNotRunning
{
  int renderCallCount = 0;
  auto player = std::make_unique<TestableIOSAudioPlayer>(
      [&renderCallCount](std::shared_ptr<DSPAudioBuffer>, int) {
        renderCallCount += 1;
      },
      48000,
      2);

  player->setRunning(false);
  for (int channel = 0; channel < 2; channel += 1) {
    for (int frame = 0; frame < static_cast<int>(player->getAudioBuffer()->getSize()); frame += 1) {
      (*player->getAudioBuffer()->getChannel(channel))[frame] = 0.75f;
    }
  }

  TestAudioOutput output(2, 64, 5.0f);
  BOOL isSilence = NO;
  NativeAudioPlayer *nativePlayer = player->getAudioPlayer();
  AudioTimeStamp timestamp = {};

  OSStatus status = nativePlayer.renderBlock(&isSilence, &timestamp, 64, output.bufferList());

  XCTAssertEqual(status, kAudioServicesNoError);
  XCTAssertEqual(renderCallCount, 0);

  for (int channel = 0; channel < 2; channel += 1) {
    for (int frame = 0; frame < 64; frame += 1) {
      XCTAssertEqualWithAccuracy(output.channels[channel][frame], 0.0f, 0.0001f);
    }
  }
}

@end

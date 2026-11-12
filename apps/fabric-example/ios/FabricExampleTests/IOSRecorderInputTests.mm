#import <AudioToolbox/AudioServices.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import <audioapi/core/OfflineAudioContext.h>
#import <audioapi/core/inputs/AudioRecorder.h>
#import <audioapi/core/sources/RecorderAdapterNode.h>
#import <audioapi/core/utils/graph/NodeHandle.h>
#import <audioapi/events/AudioEventPayload.h>
#import <audioapi/events/AudioEventProducer.h>
#import <audioapi/events/IAudioEventHandlerRegistry.h>
#import <audioapi/ios/core/IOSRecorderInput.h>
#import <audioapi/ios/system/AudioAPIError.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/utils/AudioFileProperties.h>
#import <audioapi/utils/AudioRecorderOptions.h>

#include <memory>
#include <string>
#include <vector>

using namespace audioapi;

static NSString *NSStringFromStdString(const std::string &value)
{
  return [NSString stringWithUTF8String:value.c_str()];
}

namespace audioapi {

/// Contexts take an event producer from their registry on construction, so tests need a
/// registry even when no event is ever dispatched.
class NullAudioEventHandlerRegistry : public IAudioEventHandlerRegistry {
 public:
  uint64_t registerHandler(
      AudioEvent eventName,
      const std::shared_ptr<facebook::jsi::Function> &handler) override {
    return 0;
  }

  void unregisterHandler(AudioEvent eventName, uint64_t listenerId) override {}

  bool dispatchEvent(
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept override {
    return false;
  }

  bool dispatchEventFromAudioThread(
      AudioEventProducer &producer,
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept override {
    return false;
  }

  std::shared_ptr<AudioEventProducer> createAudioEventProducer() override {
    return std::make_shared<AudioEventProducer>(producerQueue_);
  }

 private:
  /// Only ever a binding target for the producers handed out above; nothing is enqueued.
  moodycamel::ConcurrentQueue<int> producerQueue_;
};

struct RecorderAdapterTestFixture {
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<utils::graph::NodeHandle> handle;

  RecorderAdapterNode *adapter() const {
    return static_cast<RecorderAdapterNode *>(handle->audioNode.get());
  }
};

static RecorderAdapterTestFixture makeRecorderAdapterFixture() {
  RecorderAdapterTestFixture fixture;
  fixture.context = std::make_shared<OfflineAudioContext>(
      2, 512, 44100.0f, std::make_shared<NullAudioEventHandlerRegistry>());
  auto adapterNode = std::make_unique<RecorderAdapterNode>(fixture.context);
  fixture.handle =
      std::make_shared<utils::graph::NodeHandle>(std::move(adapterNode));
  return fixture;
}

/// Records every dispatched event so a test can read what reached JS.
class RecordingEventRegistry : public IAudioEventHandlerRegistry {
public:
  struct Dispatched {
    AudioEvent event;
    uint64_t listenerId;
    std::string reason;
  };

  std::vector<Dispatched> dispatched;

  uint64_t registerHandler(
      AudioEvent eventName,
      const std::shared_ptr<facebook::jsi::Function> &handler) override {
    return 0;
  }

  void unregisterHandler(AudioEvent eventName, uint64_t listenerId) override {}

  bool dispatchEvent(
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept override {
    std::string reason;
    if (auto *stringPayload = std::get_if<StringPayload>(&payload)) {
      reason = stringPayload->reason;
    }
    dispatched.push_back({eventName, listenerId, reason});
    return true;
  }

  std::shared_ptr<AudioEventProducer> createAudioEventProducer() override {
    return nullptr;
  }

  bool dispatchEventFromAudioThread(
      AudioEventProducer &producer,
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept override {
    return dispatchEvent(eventName, listenerId, std::move(payload));
  }
};

} // namespace audioapi

@interface FakeRecorderFormat : NSObject

@property(nonatomic, assign) double sampleRate;
@property(nonatomic, assign) AVAudioChannelCount channelCount;
@property(nonatomic, assign, getter=isInterleaved) BOOL interleaved;

@end

@implementation FakeRecorderFormat
@end

@interface FakeRecorderInputNode : NSObject

@property(nonatomic, strong) id inputFormat;
@property(nonatomic, strong) id outputFormat;

- (AVAudioFormat *)inputFormatForBus:(AVAudioNodeBus)bus;
- (AVAudioFormat *)outputFormatForBus:(AVAudioNodeBus)bus;

@end

@implementation FakeRecorderInputNode

- (AVAudioFormat *)inputFormatForBus:(AVAudioNodeBus)bus
{
  return (AVAudioFormat *)self.inputFormat;
}

- (AVAudioFormat *)outputFormatForBus:(AVAudioNodeBus)bus
{
  return (AVAudioFormat *)self.outputFormat;
}

@end

@interface FakeRecorderAVAudioEngine : AVAudioEngine

@property(nonatomic, strong) FakeRecorderInputNode *fakeInputNode;

@end

@implementation FakeRecorderAVAudioEngine

- (AVAudioInputNode *)inputNode
{
  return (AVAudioInputNode *)self.fakeInputNode;
}

@end

@interface FakeRecorderSharedAVAudioSession : NSObject

@property(nonatomic, assign) NSTimeInterval IOBufferDuration;
@property(nonatomic, assign) double sampleRate;

@end

@implementation FakeRecorderSharedAVAudioSession
@end

@interface FakeRecorderAudioSessionManager : AudioSessionManager

@property(nonatomic, copy) NSString *recordingPermissions;
@property(nonatomic, assign) BOOL routeReady;

@end

@implementation FakeRecorderAudioSessionManager

- (instancetype)init
{
  if (self = [super init]) {
    self.recordingPermissions = @"Granted";
    self.routeReady = YES;
  }
  return self;
}

- (NSString *)checkRecordingPermissions
{
  return self.recordingPermissions;
}

- (NSString *)inputDiagnosticsSnapshot
{
  return [NSString stringWithFormat:@"session={active=%@}; route={routeReady=%@}",
                                    self.isActive ? @"true" : @"false",
                                    self.routeReady ? @"true" : @"false"];
}

@end

@interface FakeRecorderAudioEngine : AudioEngine

@property(nonatomic, strong) FakeRecorderAVAudioEngine *fakeAVAudioEngine;
@property(nonatomic, assign) BOOL startIfNecessaryResult;
@property(nonatomic, strong) NSError *startIfNecessaryError;
@property(nonatomic, assign) NSInteger stopIfNecessaryCallCount;
@property(nonatomic, assign) NSInteger attachInputNodeCallCount;
@property(nonatomic, assign) NSInteger startIfNecessaryCallCount;
@property(nonatomic, assign) NSInteger detachInputNodeCallCount;
@property(nonatomic, assign) NSInteger stopIfPossibleCallCount;
@property(nonatomic, assign) NSInteger restartAudioEngineCallCount;
@property(nonatomic, assign) NSInteger pauseIfNecessaryCallCount;
@property(nonatomic, assign) NSInteger rebuildAfterDeactivationCallCount;
@property(nonatomic, strong) AVAudioSinkNode *lastAttachedInputNode;
@property(nonatomic, copy) AVAudioSinkNodeReceiverBlock lastAttachedReceiverBlock;
@property (nonatomic, assign) BOOL lastAttachedVoiceProcessingEnabled;
@property(nonatomic, copy) AudioEngineInputNotificationBlock lastInputNotification;
@property(nonatomic, strong) NSException *attachException;
/// Makes startIfNecessary: report HardwareChanged and RecoveryFailed before it returns. The real
/// engine reports the first from there when it has to rebuild its graph.
@property(nonatomic, assign) BOOL reportsNotificationsFromStart;

@end

@implementation FakeRecorderAudioEngine

- (void)createAudioEngineIfNeeded
{
  if (self.audioEngine != nil) {
    return;
  }

  self.fakeAVAudioEngine = [[FakeRecorderAVAudioEngine alloc] init];
  self.fakeAVAudioEngine.fakeInputNode = [[FakeRecorderInputNode alloc] init];
  self.audioEngine = self.fakeAVAudioEngine;
  self.startIfNecessaryResult = YES;
}

- (void)stopIfNecessary
{
  self.stopIfNecessaryCallCount += 1;
}

- (void)attachInputNodeWithReceiverBlock:(AVAudioSinkNodeReceiverBlock)receiverBlock
                  voiceProcessingEnabled:(BOOL)voiceProcessingEnabled
                     onInputNotification:(AudioEngineInputNotificationBlock)onInputNotification
{
  self.attachInputNodeCallCount += 1;
  self.inputNode = [[AVAudioSinkNode alloc] initWithReceiverBlock:receiverBlock];
  self.lastAttachedInputNode = self.inputNode;
  self.lastAttachedReceiverBlock = receiverBlock;
  self.lastAttachedVoiceProcessingEnabled = voiceProcessingEnabled;
  self.lastInputNotification = onInputNotification;

  if (self.attachException != nil) {
    @throw self.attachException;
  }
}

- (bool)startIfNecessary:(NSError **)error
{
  self.startIfNecessaryCallCount += 1;
  BOOL observedSessionDeactivation = self.sessionDeactivationInvalidatedGraph;

  if (observedSessionDeactivation) {
    self.rebuildAfterDeactivationCallCount += 1;
  }

  if (self.reportsNotificationsFromStart && self.lastInputNotification != nil) {
    self.lastInputNotification(AudioEngineInputNotificationHardwareChanged, nil);
    self.lastInputNotification(
        AudioEngineInputNotificationRecoveryFailed, [AudioAPIError engineStartFailed:nil]);
  }

  if (self.startIfNecessaryResult) {
    self.state = AudioEngineStateRunning;
    self.sessionDeactivationInvalidatedGraph = NO;
  } else if (error != nil) {
    // The real engine always sets the error when it fails to start.
    *error = self.startIfNecessaryError ?: [AudioAPIError engineStartFailed:nil];
  }

  return self.startIfNecessaryResult;
}

- (void)detachInputNode
{
  self.detachInputNodeCallCount += 1;
  self.inputNode = nil;
}

- (void)stopIfPossible
{
  self.stopIfPossibleCallCount += 1;
}

- (void)restartAudioEngine
{
  self.restartAudioEngineCallCount += 1;
}

- (void)pauseIfNecessary
{
  self.pauseIfNecessaryCallCount += 1;
  self.state = AudioEngineStatePaused;
}

@end

struct TestAudioInput {
  explicit TestAudioInput(UInt32 channelCount, AVAudioFrameCount frameCount)
      : storage(offsetof(AudioBufferList, mBuffers) + channelCount * sizeof(::AudioBuffer)),
        channels(channelCount, std::vector<float>(frameCount, 0.25f))
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

static id gFakeSharedRecorderSession = nil;
static BOOL gRecorderSharedSessionSwizzled = NO;

@interface AVAudioSession (IOSRecorderInputTests)

+ (id)rna_test_sharedInstance;

@end

@implementation AVAudioSession (IOSRecorderInputTests)

+ (id)rna_test_sharedInstance
{
  if (gFakeSharedRecorderSession != nil) {
    return gFakeSharedRecorderSession;
  }

  return [self rna_test_sharedInstance];
}

@end

static void SetFakeRecorderSharedAudioSession(id fakeSharedAudioSession)
{
  Method originalMethod = class_getClassMethod([AVAudioSession class], @selector(sharedInstance));
  Method swizzledMethod =
      class_getClassMethod([AVAudioSession class], @selector(rna_test_sharedInstance));

  if (!gRecorderSharedSessionSwizzled) {
    method_exchangeImplementations(originalMethod, swizzledMethod);
    gRecorderSharedSessionSwizzled = YES;
  }

  gFakeSharedRecorderSession = fakeSharedAudioSession;
}

static void ClearFakeRecorderSharedAudioSession(void)
{
  if (gRecorderSharedSessionSwizzled) {
    Method originalMethod = class_getClassMethod([AVAudioSession class], @selector(sharedInstance));
    Method swizzledMethod =
        class_getClassMethod([AVAudioSession class], @selector(rna_test_sharedInstance));
    method_exchangeImplementations(originalMethod, swizzledMethod);
    gRecorderSharedSessionSwizzled = NO;
  }

  gFakeSharedRecorderSession = nil;
}

/// IOSRecorderInput against a fake AudioEngine, driven the way the app drives it: through an
/// AudioRecorder. The session state machine itself is covered by the C++ AudioRecorderTest.
@interface IOSRecorderInputTests : XCTestCase {
 @private
  std::shared_ptr<RecordingEventRegistry> _registry;
  std::shared_ptr<AudioRecorder> _recorder;
}

@property(nonatomic, strong) FakeRecorderAudioEngine *audioEngine;
@property(nonatomic, strong) FakeRecorderAudioSessionManager *sessionManager;
@property(nonatomic, strong) FakeRecorderSharedAVAudioSession *sharedSession;

@end

@implementation IOSRecorderInputTests

- (void)setUp
{
  [super setUp];

  self.sessionManager = [[FakeRecorderAudioSessionManager alloc] init];
  self.audioEngine = [[FakeRecorderAudioEngine alloc] init];
  [self.audioEngine createAudioEngineIfNeeded];
  self.sharedSession = [[FakeRecorderSharedAVAudioSession alloc] init];
  self.sharedSession.IOBufferDuration = 0.01;
  self.sharedSession.sampleRate = 48000;
  SetFakeRecorderSharedAudioSession(self.sharedSession);

  self.audioEngine.fakeAVAudioEngine.fakeInputNode.inputFormat = [self invalidFormat];
  self.audioEngine.fakeAVAudioEngine.fakeInputNode.outputFormat = [self validFormat];

  _registry = std::make_shared<RecordingEventRegistry>();
  _recorder = [self makeRecorderWithVoiceProcessing:NO];
}

- (void)tearDown
{
  _recorder.reset();
  _registry.reset();

  [self.audioEngine cleanup];
  [self.sessionManager cleanup];
  ClearFakeRecorderSharedAudioSession();

  self.sharedSession = nil;
  self.audioEngine = nil;
  self.sessionManager = nil;

  [super tearDown];
}

- (std::shared_ptr<AudioRecorder>)makeRecorderWithVoiceProcessing:(BOOL)voiceProcessing
{
  AudioRecorderOptions options;
  options.iosVoiceProcessing = voiceProcessing;
  auto recorder =
      std::make_shared<AudioRecorder>(_registry, std::make_shared<IOSRecorderInput>(options));
  recorder->setOnErrorCallback(42);
  return recorder;
}

- (AVAudioFormat *)validFormat
{
  return [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100 channels:2];
}

- (id)invalidFormat
{
  FakeRecorderFormat *format = [[FakeRecorderFormat alloc] init];
  format.sampleRate = 0;
  format.channelCount = 0;
  format.interleaved = NO;
  return format;
}

- (std::shared_ptr<AudioFileProperties>)validFileProperties
{
  return std::make_shared<AudioFileProperties>(
      AudioFileProperties::PathConfig{
          .directory = AudioFileProperties::FileDirectory::Cache,
          .subDirectory = "fabric-example-tests",
          .fileName = "ios-recorder-test",
      },
      AudioLayout{.sampleRate = 44100, .channelCount = 2},
      AudioFileProperties::EncodingConfig{
          .format = AudioFileProperties::FileFormat::WAV,
          .bitRate = 128000,
          .bitDepth = AudioFileProperties::BitDepth::Bit16,
          .flacCompressionLevel = 0,
          .iosAudioQuality = AudioFileProperties::IOSAudioQuality::High,
      },
      AudioFileProperties::WriterConfig{
          .rotateIntervalBytes = 0,
          .androidFlushIntervalMs = 0,
      });
}

- (void)testStartStopsEngineAttachesSinkNodeAndStartsEngine
{
  XCTAssertTrue(_recorder->start().is_ok());

  XCTAssertEqual(self.audioEngine.stopIfNecessaryCallCount, 1);
  XCTAssertEqual(self.audioEngine.attachInputNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 1);
  XCTAssertNotNil(self.audioEngine.lastAttachedReceiverBlock);
  XCTAssertFalse(self.audioEngine.lastAttachedVoiceProcessingEnabled);
  XCTAssertTrue(_recorder->isRecording());
}

- (void)testStartForwardsVoiceProcessingPreferenceToAudioEngine
{
  auto recorder = [self makeRecorderWithVoiceProcessing:YES];

  XCTAssertTrue(recorder->start().is_ok());

  XCTAssertEqual(self.audioEngine.attachInputNodeCallCount, 1);
  XCTAssertTrue(self.audioEngine.lastAttachedVoiceProcessingEnabled);
}

- (void)testStartFailsWhenRecordingPermissionIsDenied
{
  self.sessionManager.recordingPermissions = @"Denied";

  auto result = _recorder->start();

  XCTAssertTrue(result.is_err());
  XCTAssertEqualObjects(
      NSStringFromStdString(result.unwrap_err()), @"Microphone permissions are not granted");
  XCTAssertEqual(self.audioEngine.attachInputNodeCallCount, 0);
  XCTAssertTrue(_recorder->isIdle());
}

- (void)testStartFailsAndDetachesInputWhenEngineFailsToStart
{
  self.audioEngine.startIfNecessaryResult = NO;
  self.audioEngine.startIfNecessaryError =
      [NSError errorWithDomain:@"RecorderTests"
                          code:7
                      userInfo:@{NSLocalizedDescriptionKey : @"boom"}];

  auto result = _recorder->start();

  XCTAssertTrue(result.is_err());
  NSString *message = NSStringFromStdString(result.unwrap_err());
  XCTAssertTrue([message containsString:@"Failed to start native recorder"], @"%@", message);
  XCTAssertTrue([message containsString:@"RecorderTests"], @"%@", message);
  XCTAssertEqual(self.audioEngine.detachInputNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.stopIfPossibleCallCount, 1);
  XCTAssertTrue(_recorder->isIdle());
}

- (void)testFailedStartRestartsTheEngineForPlaybackThatWasRunning
{
  self.audioEngine.state = AudioEngineStateRunning;
  self.audioEngine.startIfNecessaryResult = NO;

  XCTAssertTrue(_recorder->start().is_err());

  // Once with the input attached, once more without it.
  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 2);
  XCTAssertEqual(self.audioEngine.detachInputNodeCallCount, 1);
  XCTAssertNil(self.audioEngine.inputNode);
}

- (void)testFailedStartLeavesAnEngineThatWasNotRunningStopped
{
  self.audioEngine.startIfNecessaryResult = NO;

  XCTAssertTrue(_recorder->start().is_err());

  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 1);
}

- (void)testStartFailsAndDetachesInputWhenEngineThrows
{
  self.audioEngine.attachException =
      [NSException exceptionWithName:@"WrongCategory"
                              reason:@"attempt-wrong-category-record"
                            userInfo:nil];

  auto result = _recorder->start();

  XCTAssertTrue(result.is_err());
  NSString *message = NSStringFromStdString(result.unwrap_err());
  XCTAssertTrue([message containsString:@"Failed to start native recorder"], @"%@", message);
  XCTAssertTrue([message containsString:@"WrongCategory"], @"%@", message);
  XCTAssertTrue([message containsString:@"attempt-wrong-category-record"], @"%@", message);
  XCTAssertTrue([message containsString:@"session={"], @"%@", message);
  XCTAssertEqual(self.audioEngine.detachInputNodeCallCount, 1);
  XCTAssertTrue(_recorder->isIdle());
}

- (void)testStartFailsAndDetachesInputWhenEngineInputFormatIsUnavailable
{
  self.audioEngine.fakeAVAudioEngine.fakeInputNode.outputFormat = [self invalidFormat];
  self.sessionManager.routeReady = NO;

  auto result = _recorder->start();

  XCTAssertTrue(result.is_err());
  NSString *message = NSStringFromStdString(result.unwrap_err());
  XCTAssertTrue([message containsString:@"Audio input format is unavailable"], @"%@", message);
  XCTAssertTrue([message containsString:@"routeReady=false"], @"%@", message);
  XCTAssertEqual(self.audioEngine.detachInputNodeCallCount, 1);
  XCTAssertTrue(_recorder->isIdle());
}

- (void)testStartFailsAndDetachesInputWhenTheInputIsInterleavedMultichannel
{
  self.audioEngine.fakeAVAudioEngine.fakeInputNode.outputFormat =
      [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                       sampleRate:44100
                                         channels:2
                                      interleaved:YES];

  auto result = _recorder->start();

  XCTAssertTrue(result.is_err());
  NSString *message = NSStringFromStdString(result.unwrap_err());
  XCTAssertTrue(
      [message containsString:@"Interleaved multi-channel input is not supported"], @"%@", message);
  XCTAssertEqual(self.audioEngine.detachInputNodeCallCount, 1);
  XCTAssertTrue(_recorder->isIdle());
}

- (void)testStopDetachesInputAndRestartsEngine
{
  XCTAssertTrue(_recorder->start().is_ok());

  XCTAssertTrue(_recorder->stop().is_ok());

  XCTAssertEqual(self.audioEngine.detachInputNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.stopIfPossibleCallCount, 1);
  XCTAssertEqual(self.audioEngine.restartAudioEngineCallCount, 1);
  XCTAssertTrue(_recorder->isIdle());
}

- (void)testPauseAndResumeDelegateToAudioEngine
{
  XCTAssertTrue(_recorder->start().is_ok());

  _recorder->pause();

  XCTAssertEqual(self.audioEngine.pauseIfNecessaryCallCount, 1);
  XCTAssertTrue(_recorder->isPaused());

  XCTAssertTrue(_recorder->resume().is_ok());

  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 2);
  XCTAssertTrue(_recorder->isRecording());
}

- (void)testResumeReturnsTheEngineErrorWithoutReportingIt
{
  XCTAssertTrue(_recorder->start().is_ok());
  _recorder->pause();
  // The code iOS returns when a non-mixable session may not go active in the background.
  self.audioEngine.startIfNecessaryResult = NO;
  self.audioEngine.startIfNecessaryError = [AudioAPIError
      sessionActivationFailed:[NSError errorWithDomain:NSOSStatusErrorDomain
                                                  code:560557684
                                              userInfo:nil]];

  auto result = _recorder->resume();

  XCTAssertTrue(result.is_err());
  NSString *reason = NSStringFromStdString(result.unwrap_err());
  XCTAssertTrue([reason containsString:@"Failed to resume"], @"%@", reason);
  XCTAssertTrue([reason containsString:@"560557684"], @"%@", reason);
  XCTAssertTrue(_recorder->isPaused());
  XCTAssertEqual(_registry->dispatched.size(), 0UL);
}

- (void)testAnEngineThatIsNotRunningReadsAsPaused
{
  XCTAssertTrue(_recorder->start().is_ok());

  self.audioEngine.state = AudioEngineStateInterrupted;

  XCTAssertTrue(_recorder->isPaused());
  XCTAssertFalse(_recorder->isRecording());
}

- (void)testRecoveryFailedNotificationReportsThroughRecorderError
{
  XCTAssertTrue(_recorder->start().is_ok());
  NSError *activationRefused = [AudioAPIError
      sessionActivationFailed:[NSError errorWithDomain:NSOSStatusErrorDomain
                                                  code:560557684
                                              userInfo:nil]];

  self.audioEngine.lastInputNotification(
      AudioEngineInputNotificationRecoveryFailed, activationRefused);

  XCTAssertEqual(_registry->dispatched.size(), 1UL);
  XCTAssertEqual(_registry->dispatched[0].event, AudioEvent::RECORDER_ERROR);
  XCTAssertEqual(_registry->dispatched[0].listenerId, 42ULL);
  NSString *reason = NSStringFromStdString(_registry->dispatched[0].reason);
  XCTAssertTrue([reason containsString:@"could not be recovered"], @"%@", reason);
  XCTAssertTrue([reason containsString:@"560557684"], @"%@", reason);
}

- (void)testNotificationsOfAStoppedSessionAreDropped
{
  XCTAssertTrue(_recorder->start().is_ok());
  AudioEngineInputNotificationBlock staleNotification = self.audioEngine.lastInputNotification;
  XCTAssertTrue(_recorder->stop().is_ok());

  staleNotification(AudioEngineInputNotificationHardwareChanged, nil);
  staleNotification(AudioEngineInputNotificationRecoveryFailed, [AudioAPIError engineStartFailed:nil]);

  XCTAssertEqual(_registry->dispatched.size(), 0UL);
  XCTAssertTrue(_recorder->isIdle());
}

- (void)testHardwareChangeNotificationRepreparesOutputsForTheNewInputFormat
{
  auto adapterFixture = makeRecorderAdapterFixture();
  _recorder->connect(adapterFixture.handle);
  XCTAssertTrue(_recorder->start().is_ok());
  XCTAssertEqual(adapterFixture.adapter()->buff_.size(), 2U);

  self.audioEngine.fakeAVAudioEngine.fakeInputNode.outputFormat =
      [[AVAudioFormat alloc] initStandardFormatWithSampleRate:32000 channels:1];
  self.audioEngine.lastInputNotification(AudioEngineInputNotificationHardwareChanged, nil);

  XCTAssertEqual(adapterFixture.adapter()->buff_.size(), 1U);
  XCTAssertTrue(_recorder->isRecording());
  XCTAssertEqual(_registry->dispatched.size(), 0UL);
}

/// The engine reports from inside the input's own startIfNecessary:, where the recorder already
/// holds its control mutex. Forwarding a notification from there would deadlock, so the session
/// runs off the test thread and is given a deadline.
- (void)testNotificationsReportedFromInsideAnOwnEngineCallAreNotForwarded
{
  auto adapterFixture = makeRecorderAdapterFixture();
  _recorder->connect(adapterFixture.handle);
  self.audioEngine.reportsNotificationsFromStart = YES;

  XCTestExpectation *sessionReturned =
      [self expectationWithDescription:@"start, pause and resume return"];
  // Captured by value: after a deadlock the stuck thread keeps the recorder alive, so tearDown
  // does not block on the same mutex.
  auto recorder = _recorder;
  FakeRecorderInputNode *inputNode = self.audioEngine.fakeAVAudioEngine.fakeInputNode;
  __block BOOL didResume = NO;
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    if (recorder->start().is_ok()) {
      recorder->pause();
      inputNode.outputFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:32000
                                                                              channels:1];
      didResume = recorder->resume().is_ok();
    }
    [sessionReturned fulfill];
  });
  [self waitForExpectations:@[ sessionReturned ] timeout:5];

  XCTAssertTrue(didResume);
  // The format HardwareChanged announced is read once the engine call has returned.
  XCTAssertEqual(adapterFixture.adapter()->buff_.size(), 1U);
  XCTAssertEqual(_registry->dispatched.size(), 0UL);
}

- (void)testStartAfterSessionDeactivationUsesRecoveryRebuildPath
{
  XCTAssertTrue(_recorder->start().is_ok());

  [self.audioEngine onSessionDeactivated];
  XCTAssertTrue(self.audioEngine.sessionDeactivationInvalidatedGraph);
  XCTAssertTrue(_recorder->stop().is_ok());

  XCTAssertTrue(_recorder->start().is_ok());

  XCTAssertEqual(self.audioEngine.rebuildAfterDeactivationCallCount, 1);
  XCTAssertFalse(self.audioEngine.sessionDeactivationInvalidatedGraph);
}

- (void)testConnectWhileRecordingInitializesAdapterAndDisconnectClearsIt
{
  auto adapterFixture = makeRecorderAdapterFixture();
  XCTAssertTrue(_recorder->start().is_ok());
  XCTAssertEqual(adapterFixture.adapter()->buff_.size(), 0U);

  _recorder->connect(adapterFixture.handle);

  XCTAssertTrue(_recorder->isConnected());
  XCTAssertEqual(adapterFixture.adapter()->buff_.size(), 2U);

  _recorder->disconnect();

  XCTAssertFalse(_recorder->isConnected());
}

- (void)testReceiverBlockFeedsTheRecordedFile
{
  NSString *uuid = [[NSUUID UUID] UUIDString];
  auto properties = [self validFileProperties];
  properties->path.fileName =
      [[NSString stringWithFormat:@"ios-recorder-input-%@", uuid] UTF8String];
  XCTAssertTrue(_recorder->enableFileOutput(properties).is_ok());
  XCTAssertTrue(_recorder->start().is_ok());

  TestAudioInput input(2, 512);
  AudioTimeStamp timestamp = {};
  for (int block = 0; block < 8; ++block) {
    OSStatus status = self.audioEngine.lastAttachedReceiverBlock(&timestamp, 512, input.bufferList());
    XCTAssertEqual(status, (OSStatus)kAudioServicesNoError);
  }

  auto stopResult = _recorder->stop();

  XCTAssertTrue(stopResult.is_ok());
  const auto &recording = stopResult.unwrap();
  XCTAssertEqual(recording.paths.size(), 1U);
  NSString *uri = NSStringFromStdString(recording.paths.front());
  XCTAssertTrue([uri hasPrefix:@"file://"], @"%@", uri);
  NSString *path = [uri substringFromIndex:@"file://".length];
  XCTAssertTrue([[NSFileManager defaultManager] fileExistsAtPath:path]);
  XCTAssertGreaterThan(recording.duration, 0.0);

  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
}

@end

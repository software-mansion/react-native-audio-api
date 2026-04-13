#import <XCTest/XCTest.h>

#import <audioapi/core/OfflineAudioContext.h>
#import <audioapi/ios/core/NativeAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/core/inputs/AudioRecorder.h>
#import <audioapi/core/sources/RecorderAdapterNode.h>
#import <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <string>
#include <tuple>

using namespace audioapi;

static NSString *NSStringFromStdString(const std::string &value)
{
  return [NSString stringWithUTF8String:value.c_str()];
}

namespace audioapi {

class AudioEventHandlerRegistry;

class IOSAudioRecorder : public AudioRecorder {
 public:
  IOSAudioRecorder(const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~IOSAudioRecorder() override;

  Result<NoneType, std::string> start(const std::string &fileNameOverride = "") override;
  Result<std::tuple<std::vector<std::string>, double, double>, std::string> stop() override;

  Result<NoneType, std::string> enableFileOutput(
      std::shared_ptr<AudioFileProperties> properties) override;
  void disableFileOutput() override;

  void connect(const std::shared_ptr<RecorderAdapterNode> &node) override;
  void disconnect() override;

  void pause() override;
  void resume() override;

  bool isRecording() const override;
  bool isPaused() const override;
  bool isIdle() const override;

  Result<NoneType, std::string> setOnAudioReadyCallback(
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId) override;
  void clearOnAudioReadyCallback() override;

 protected:
  NativeAudioRecorder *nativeRecorder_;
};

} // namespace audioapi

@interface FakeIOSRecorderFormat : NSObject

@property(nonatomic, assign) double sampleRate;
@property(nonatomic, assign) AVAudioChannelCount channelCount;
@property(nonatomic, assign, getter=isInterleaved) BOOL interleaved;

@end

@implementation FakeIOSRecorderFormat
@end

@interface FakeIOSRecorderAudioEngine : AudioEngine
@end

@implementation FakeIOSRecorderAudioEngine

- (void)createAudioEngineIfNeeded
{
  // Avoid spinning up a real AVAudioEngine inside unit tests.
}

@end

@interface FakeIOSRecorderAudioSessionManager : AudioSessionManager

@property(nonatomic, copy) NSString *recordingPermissions;
@property(nonatomic, assign) double diagnosticSampleRate;
@property(nonatomic, assign) AVAudioChannelCount diagnosticInputChannels;
@property(nonatomic, assign) BOOL routeReady;

@end

@implementation FakeIOSRecorderAudioSessionManager

- (instancetype)init
{
  if (self = [super init]) {
    self.recordingPermissions = @"Granted";
    self.diagnosticSampleRate = 44100;
    self.diagnosticInputChannels = 2;
    self.routeReady = YES;
  }

  return self;
}

- (NSString *)checkRecordingPermissions
{
  return self.recordingPermissions;
}

- (bool)ensureActive:(bool)force error:(NSError **)error
{
  if (error != nil) {
    *error = nil;
  }
  return true;
}

- (NSString *)inputDiagnosticsSnapshot
{
  return [NSString stringWithFormat:@"session={active=%@, sampleRate=%f, inputChannels=%lu}; "
                                   @"route={routeReady=%@}",
                                   self.isActive ? @"true" : @"false",
                                   self.diagnosticSampleRate,
                                   (unsigned long)self.diagnosticInputChannels,
                                   self.routeReady ? @"true" : @"false"];
}

@end

@interface FakeNativeAudioRecorder : NativeAudioRecorder

@property(nonatomic, strong) id mockResolvedInputFormat;
@property(nonatomic, assign) int mockResolvedBufferSize;
@property(nonatomic, assign) BOOL startResult;
@property(nonatomic, strong) NSError *startError;
@property(nonatomic, assign) NSInteger startCallCount;
@property(nonatomic, assign) NSInteger stopCallCount;
@property(nonatomic, assign) NSInteger pauseCallCount;
@property(nonatomic, assign) NSInteger resumeCallCount;
@property(nonatomic, assign) NSInteger cleanupCallCount;
@property(nonatomic, assign) NSInteger setInputArmedCallCount;
@property(nonatomic, assign) BOOL lastInputArmed;
@property(nonatomic, assign) BOOL throwsOnStart;
@property(nonatomic, strong) NSException *startException;

@end

@implementation FakeNativeAudioRecorder

- (instancetype)init
{
  if (self = [super initWithReceiverBlock:^(const AudioBufferList *inputBuffer, int numFrames) {
      }]) {
    self.mockResolvedInputFormat =
        [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100 channels:2];
    self.mockResolvedBufferSize = 512;
    self.startResult = YES;
  }

  return self;
}

- (AVAudioFormat *)getResolvedInputFormat
{
  return (AVAudioFormat *)self.mockResolvedInputFormat;
}

- (int)getResolvedBufferSize
{
  return self.mockResolvedBufferSize;
}

- (BOOL)start:(NSError **)error
{
  self.startCallCount += 1;

  if (self.throwsOnStart) {
    @throw self.startException ?: [NSException exceptionWithName:@"FakeStartException"
                                                          reason:@"boom"
                                                        userInfo:nil];
  }

  if (error != nil) {
    *error = self.startError;
  }

  return self.startResult;
}

- (void)setInputArmed:(BOOL)armed
{
  self.setInputArmedCallCount += 1;
  self.lastInputArmed = armed;
  [super setInputArmed:armed];
}

- (void)stop
{
  self.stopCallCount += 1;
}

- (void)pause
{
  self.pauseCallCount += 1;
}

- (void)resume
{
  self.resumeCallCount += 1;
}

- (void)cleanup
{
  self.cleanupCallCount += 1;
  [super cleanup];
}

@end

class TestableIOSAudioRecorder : public IOSAudioRecorder {
 public:
  explicit TestableIOSAudioRecorder(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
      : IOSAudioRecorder(audioEventHandlerRegistry) {}

  NativeAudioRecorder *replaceNativeRecorder(NativeAudioRecorder *nativeRecorder)
  {
    NativeAudioRecorder *previous = nativeRecorder_;
    nativeRecorder_ = nativeRecorder;
    return previous;
  }

  void setRecorderState(RecorderState state)
  {
    state_.store(state, std::memory_order_release);
  }

  std::string currentFilePath() const
  {
    return filePath_;
  }

  bool fileOutputEnabledIntent() const
  {
    return fileOutputEnabled_.load(std::memory_order_acquire);
  }

  bool fileOutputConfigured() const
  {
    return fileOutputConfigured_.load(std::memory_order_acquire);
  }

  bool callbackOutputEnabledIntent() const
  {
    return callbackOutputEnabled_.load(std::memory_order_acquire);
  }

  bool callbackOutputConfigured() const
  {
    return callbackOutputConfigured_.load(std::memory_order_acquire);
  }

  bool connectionEnabledIntent() const
  {
    return isConnected_.load(std::memory_order_acquire);
  }

  bool connectionConfigured() const
  {
    return connectedConfigured_.load(std::memory_order_acquire);
  }
};

@interface IOSAudioRecorderTests : XCTestCase {
 @private
  std::unique_ptr<TestableIOSAudioRecorder> _recorder;
}

@property(nonatomic, strong) FakeIOSRecorderAudioEngine *audioEngine;
@property(nonatomic, strong) FakeIOSRecorderAudioSessionManager *sessionManager;
@property(nonatomic, strong) FakeNativeAudioRecorder *nativeRecorder;
@property(nonatomic, assign) NativeAudioRecorder *originalNativeRecorder;

@end

@implementation IOSAudioRecorderTests

- (void)setUp
{
  [super setUp];

  self.sessionManager = [[FakeIOSRecorderAudioSessionManager alloc] init];
  self.audioEngine = [[FakeIOSRecorderAudioEngine alloc] init];
  self.nativeRecorder = [[FakeNativeAudioRecorder alloc] init];

  _recorder = std::make_unique<TestableIOSAudioRecorder>(std::shared_ptr<AudioEventHandlerRegistry>());
  self.originalNativeRecorder = _recorder->replaceNativeRecorder(self.nativeRecorder);
}

- (void)tearDown
{
  if (self.originalNativeRecorder != nil) {
    [self.originalNativeRecorder cleanup];
  }

  _recorder.reset();

  [self.audioEngine cleanup];
  [self.sessionManager cleanup];

  self.originalNativeRecorder = nil;
  self.nativeRecorder = nil;
  self.audioEngine = nil;
  self.sessionManager = nil;

  [super tearDown];
}

- (std::shared_ptr<AudioFileProperties>)validFileProperties
{
  return std::make_shared<AudioFileProperties>(
      AudioFileProperties::FileDirectory::Cache,
      "fabric-example-tests",
      "ios-recorder-test",
      2,
      1,
      0,
      AudioFileProperties::Format::WAV,
      44100,
      128000,
      AudioFileProperties::BitDepth::Bit16,
      0,
      0,
      AudioFileProperties::IOSAudioQuality::High);
}

- (id)invalidFormat
{
  FakeIOSRecorderFormat *format = [[FakeIOSRecorderFormat alloc] init];
  format.sampleRate = 0;
  format.channelCount = 0;
  format.interleaved = NO;
  return format;
}

- (id)validMultichannelFormat
{
  AVAudioChannelLayout *layout =
      [[AVAudioChannelLayout alloc] initWithLayoutTag:kAudioChannelLayoutTag_MPEG_7_1_A];
  return [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                          sampleRate:44100
                                         interleaved:NO
                                       channelLayout:layout];
}

- (void)testStartReturnsErrorWhenRecorderIsNotIdle
{
  _recorder->setRecorderState(AudioRecorder::RecorderState::Paused);

  auto result = _recorder->start("");

  XCTAssertTrue(result.is_err());
  XCTAssertEqualObjects(NSStringFromStdString(result.unwrap_err()), @"Recorder is already recording");
}

- (void)testStartReturnsErrorWhenRecordingPermissionIsDenied
{
  self.sessionManager.recordingPermissions = @"Denied";

  auto result = _recorder->start("");

  XCTAssertTrue(result.is_err());
  XCTAssertEqualObjects(
      NSStringFromStdString(result.unwrap_err()),
      @"Microphone permissions are not granted");
}

- (void)testStartReturnsErrorWhenSessionActivationFails
{
  self.nativeRecorder.startResult = NO;
  self.nativeRecorder.startError =
      [NSError errorWithDomain:@"RecorderTests" code:7 userInfo:@{NSLocalizedDescriptionKey : @"boom"}];

  auto result = _recorder->start("");

  XCTAssertTrue(result.is_err());
  NSString *message = NSStringFromStdString(result.unwrap_err());
  XCTAssertTrue([message containsString:@"Failed to start native recorder"]);
  XCTAssertTrue([message containsString:@"RecorderTests"]);
}

- (void)testStartReturnsErrorWhenNativeRecorderThrows
{
  self.nativeRecorder.throwsOnStart = YES;
  self.nativeRecorder.startException = [NSException exceptionWithName:@"WrongCategory"
                                                               reason:@"attempt-wrong-category-record"
                                                             userInfo:nil];

  auto result = _recorder->start("");

  XCTAssertTrue(result.is_err());
  XCTAssertEqual(self.nativeRecorder.startCallCount, 1);
  XCTAssertEqual(self.nativeRecorder.stopCallCount, 1);

  NSString *message = NSStringFromStdString(result.unwrap_err());
  XCTAssertTrue([message containsString:@"Failed to start native recorder"]);
  XCTAssertTrue([message containsString:@"WrongCategory"]);
  XCTAssertTrue([message containsString:@"attempt-wrong-category-record"]);
  XCTAssertTrue([message containsString:@"session={"]);
}

- (void)testStartReturnsErrorWhenEngineInputFormatIsUnavailable
{
  self.nativeRecorder.mockResolvedInputFormat = [self invalidFormat];
  self.sessionManager.diagnosticSampleRate = 0;
  self.sessionManager.diagnosticInputChannels = 0;
  self.sessionManager.routeReady = NO;

  auto result = _recorder->start("");

  XCTAssertTrue(result.is_err());
  XCTAssertEqual(self.nativeRecorder.startCallCount, 1);
  XCTAssertEqual(self.nativeRecorder.stopCallCount, 1);

  NSString *message = NSStringFromStdString(result.unwrap_err());
  XCTAssertTrue([message containsString:@"Audio input format is unavailable"]);
  XCTAssertTrue([message containsString:@"engineFormat={sampleRate=0.000000, channelCount=0"]);
  XCTAssertTrue([message containsString:@"sampleRate=0.000000"]);
  XCTAssertTrue([message containsString:@"inputChannels=0"]);
  XCTAssertTrue([message containsString:@"routeReady=false"]);
}

- (void)testStartSucceedsWhenResolvedInputFormatIsAvailable
{
  self.audioEngine.state = AudioEngineStateRunning;
  self.nativeRecorder.mockResolvedInputFormat = [self validMultichannelFormat];

  auto result = _recorder->start("");

  XCTAssertTrue(result.is_ok());
  XCTAssertEqual(self.nativeRecorder.startCallCount, 1);
  XCTAssertFalse(_recorder->isIdle());
  XCTAssertTrue(self.nativeRecorder.lastInputArmed);
  XCTAssertGreaterThanOrEqual(self.nativeRecorder.setInputArmedCallCount, 1);
  XCTAssertFalse(_recorder->isIdle());
  XCTAssertEqual(_recorder->currentFilePath(), "");
}

- (void)testStartPreparesMonoCallbackAgainstResolvedMultichannelInputFormat
{
  self.audioEngine.state = AudioEngineStateRunning;
  self.nativeRecorder.mockResolvedInputFormat = [self validMultichannelFormat];

  auto callbackResult = _recorder->setOnAudioReadyCallback(48000, 256, 1, 99);
  XCTAssertTrue(callbackResult.is_ok());

  auto startResult = _recorder->start("");

  XCTAssertTrue(startResult.is_ok());
  XCTAssertTrue(_recorder->usesCallback());
  XCTAssertTrue(self.nativeRecorder.lastInputArmed);
}

- (void)testEnableFileOutputWhileIdleTracksIntentWithoutLiveWriter
{
  auto enableResult = _recorder->enableFileOutput([self validFileProperties]);

  XCTAssertTrue(enableResult.is_ok());
  XCTAssertTrue(_recorder->fileOutputEnabledIntent());
  XCTAssertFalse(_recorder->fileOutputConfigured());
  XCTAssertFalse(_recorder->usesFileOutput());
  XCTAssertEqual(_recorder->getCurrentDuration(), 0.0);

  _recorder->clearOnErrorCallback();
}

- (void)testSetOnAudioReadyWhileIdleTracksIntentWithoutLiveCallback
{
  auto callbackResult = _recorder->setOnAudioReadyCallback(48000, 256, 1, 99);

  XCTAssertTrue(callbackResult.is_ok());
  XCTAssertTrue(_recorder->callbackOutputEnabledIntent());
  XCTAssertFalse(_recorder->callbackOutputConfigured());
  XCTAssertFalse(_recorder->usesCallback());
}

- (void)testConnectWhileIdleTracksIntentWithoutLiveConnection
{
  auto context =
      std::make_shared<OfflineAudioContext>(2, 512, 44100.0f, nullptr, RuntimeRegistry{});
  context->initialize();
  auto adapter = context->createRecorderAdapter();

  _recorder->connect(adapter);

  XCTAssertTrue(_recorder->connectionEnabledIntent());
  XCTAssertFalse(_recorder->connectionConfigured());
  XCTAssertFalse(_recorder->isConnected());
}

- (void)testStartDoesNotAttemptToManageSessionWhenOwnershipIsExternal
{
  self.sessionManager.shouldManageSession = NO;

  auto result = _recorder->start("");

  XCTAssertTrue(result.is_ok());
  XCTAssertEqual(self.nativeRecorder.startCallCount, 1);
}

- (void)testPauseAndResumeRespectCurrentState
{
  _recorder->pause();
  _recorder->resume();

  XCTAssertEqual(self.nativeRecorder.pauseCallCount, 0);
  XCTAssertEqual(self.nativeRecorder.resumeCallCount, 0);

  _recorder->setRecorderState(AudioRecorder::RecorderState::Recording);
  self.audioEngine.state = AudioEngineStateRunning;
  _recorder->pause();

  XCTAssertEqual(self.nativeRecorder.pauseCallCount, 1);
  XCTAssertTrue(_recorder->isPaused());

  _recorder->resume();

  XCTAssertEqual(self.nativeRecorder.resumeCallCount, 1);
  XCTAssertFalse(_recorder->isPaused());
}

- (void)testStopReturnsErrorWhileIdle
{
  auto result = _recorder->stop();

  XCTAssertTrue(result.is_err());
  XCTAssertEqualObjects(
      NSStringFromStdString(result.unwrap_err()),
      @"Recorder is not in recording state.");
}

- (void)testStopSucceedsAfterStartAndResetsState
{
  self.audioEngine.state = AudioEngineStateRunning;
  auto startResult = _recorder->start("");
  XCTAssertTrue(startResult.is_ok());

  auto stopResult = _recorder->stop();

  XCTAssertTrue(stopResult.is_ok());
  XCTAssertEqual(self.nativeRecorder.stopCallCount, 1);
  XCTAssertTrue(_recorder->isIdle());
  XCTAssertEqual(_recorder->currentFilePath(), "");
  XCTAssertTrue(std::get<0>(stopResult.unwrap()).empty());
  XCTAssertEqual(std::get<1>(stopResult.unwrap()), 0);
  XCTAssertEqual(std::get<2>(stopResult.unwrap()), 0);
}

- (void)testStopClearsConfiguredStateButPreservesConfiguredIntent
{
  self.audioEngine.state = AudioEngineStateRunning;
  auto context =
      std::make_shared<OfflineAudioContext>(2, 512, 44100.0f, nullptr, RuntimeRegistry{});
  context->initialize();
  auto adapter = context->createRecorderAdapter();

  XCTAssertTrue(_recorder->enableFileOutput([self validFileProperties]).is_ok());
  XCTAssertTrue(_recorder->setOnAudioReadyCallback(48000, 256, 1, 99).is_ok());
  _recorder->connect(adapter);

  auto startResult = _recorder->start("");
  XCTAssertTrue(startResult.is_ok());
  XCTAssertTrue(_recorder->fileOutputConfigured());
  XCTAssertTrue(_recorder->callbackOutputConfigured());
  XCTAssertTrue(_recorder->connectionConfigured());

  auto stopResult = _recorder->stop();

  XCTAssertTrue(stopResult.is_ok());
  XCTAssertTrue(_recorder->fileOutputEnabledIntent());
  XCTAssertFalse(_recorder->fileOutputConfigured());
  XCTAssertFalse(_recorder->usesFileOutput());
  XCTAssertTrue(_recorder->callbackOutputEnabledIntent());
  XCTAssertFalse(_recorder->callbackOutputConfigured());
  XCTAssertFalse(_recorder->usesCallback());
  XCTAssertTrue(_recorder->connectionEnabledIntent());
  XCTAssertFalse(_recorder->connectionConfigured());
  XCTAssertFalse(_recorder->isConnected());
}

- (void)testFileOutputSmokeTest
{
  self.audioEngine.state = AudioEngineStateRunning;
  auto enableResult = _recorder->enableFileOutput([self validFileProperties]);
  XCTAssertTrue(enableResult.is_ok());

  NSString *uuid = [[NSUUID UUID] UUIDString];
  std::string fileName = [[NSString stringWithFormat:@"ios-recorder-smoke-%@", uuid] UTF8String];

  auto startResult = _recorder->start(fileName);
  XCTAssertTrue(startResult.is_ok());

  NSString *path = NSStringFromStdString(_recorder->currentFilePath());
  XCTAssertFalse(path.length == 0);
  XCTAssertTrue([[NSFileManager defaultManager] fileExistsAtPath:path]);

  auto stopResult = _recorder->stop();
  XCTAssertTrue(stopResult.is_ok());
  const auto &outputPaths = std::get<0>(stopResult.unwrap());
  XCTAssertEqual(outputPaths.size(), 1U);
  XCTAssertEqualObjects(
      NSStringFromStdString(outputPaths.front()),
      [@"file://" stringByAppendingString:path]);
  XCTAssertGreaterThanOrEqual(std::get<1>(stopResult.unwrap()), 0.0);
  XCTAssertGreaterThanOrEqual(std::get<2>(stopResult.unwrap()), 0.0);
  XCTAssertEqual(_recorder->currentFilePath(), "");

  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
}

- (void)testStartReturnsCallbackPreparationFailureForInvalidCallbackFormat
{
  auto callbackResult = _recorder->setOnAudioReadyCallback(0, 256, 2, 99);
  XCTAssertTrue(callbackResult.is_ok());

  auto result = _recorder->start("");

  XCTAssertTrue(result.is_err());
  XCTAssertTrue(
      [NSStringFromStdString(result.unwrap_err())
          containsString:@"Failed to prepare callback: Invalid callback format"]);
}

- (void)testConnectWhileActiveInitializesAdapterAndDisconnectClearsIt
{
  auto context =
      std::make_shared<OfflineAudioContext>(2, 512, 44100.0f, nullptr, RuntimeRegistry{});
  context->initialize();
  auto adapter = context->createRecorderAdapter();

  XCTAssertFalse(_recorder->isConnected());
  XCTAssertEqual(adapter->buff_.size(), 0U);

  _recorder->setRecorderState(AudioRecorder::RecorderState::Recording);
  _recorder->connect(adapter);

  XCTAssertTrue(_recorder->isConnected());
  XCTAssertEqual(adapter->buff_.size(), 2U);
  XCTAssertTrue(adapter->buff_[0] != nullptr);
  XCTAssertTrue(adapter->buff_[1] != nullptr);

  _recorder->disconnect();

  XCTAssertFalse(_recorder->isConnected());
}

@end

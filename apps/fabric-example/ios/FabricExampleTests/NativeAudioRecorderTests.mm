#import <AudioToolbox/AudioServices.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import <audioapi/ios/core/NativeAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

#include <vector>

@interface FakeRecorderFormat : NSObject

@property(nonatomic, assign) double sampleRate;
@property(nonatomic, assign) AVAudioChannelCount channelCount;
@property(nonatomic, assign, getter=isInterleaved) BOOL interleaved;

@end

@implementation FakeRecorderFormat
@end

@interface FakeRecorderInputNode : NSObject

@property(nonatomic, strong) id format;

- (AVAudioFormat *)inputFormatForBus:(AVAudioNodeBus)bus;

@end

@implementation FakeRecorderInputNode

- (AVAudioFormat *)inputFormatForBus:(AVAudioNodeBus)bus
{
  return (AVAudioFormat *)self.format;
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

@property(nonatomic, strong) AVAudioFormat *preferredInputFormat;

@end

@implementation FakeRecorderAudioSessionManager

- (AVAudioFormat *)getPreferredInputFormat
{
  return self.preferredInputFormat;
}

@end

@interface FakeRecorderAudioEngine : AudioEngine

@property(nonatomic, strong) FakeRecorderAVAudioEngine *fakeAVAudioEngine;
@property(nonatomic, assign) NSInteger stopIfNecessaryCallCount;
@property(nonatomic, assign) NSInteger attachInputNodeCallCount;
@property(nonatomic, assign) NSInteger startIfNecessaryCallCount;
@property(nonatomic, assign) NSInteger detachInputNodeCallCount;
@property(nonatomic, assign) NSInteger stopIfPossibleCallCount;
@property(nonatomic, assign) NSInteger restartAudioEngineCallCount;
@property(nonatomic, assign) NSInteger pauseIfNecessaryCallCount;
@property(nonatomic, strong) AVAudioSinkNode *lastAttachedInputNode;
@property(nonatomic, strong) AVAudioFormat *lastAttachedFormat;

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
}

- (void)stopIfNecessary
{
  self.stopIfNecessaryCallCount += 1;
}

- (void)attachInputNode:(AVAudioSinkNode *)inputNode format:(AVAudioFormat *)format
{
  self.attachInputNodeCallCount += 1;
  self.inputNode = inputNode;
  self.lastAttachedInputNode = inputNode;
  self.lastAttachedFormat = format;
}

- (bool)startIfNecessary
{
  self.startIfNecessaryCallCount += 1;
  self.state = AudioEngineStateRunning;
  return true;
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

@interface AVAudioSession (NativeAudioRecorderTests)

+ (id)rna_test_sharedInstance;

@end

@implementation AVAudioSession (NativeAudioRecorderTests)

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

@interface NativeAudioRecorderTests : XCTestCase

@property(nonatomic, strong) FakeRecorderAudioEngine *audioEngine;
@property(nonatomic, strong) FakeRecorderAudioSessionManager *sessionManager;
@property(nonatomic, strong) FakeRecorderSharedAVAudioSession *sharedSession;

@end

@implementation NativeAudioRecorderTests

- (void)setUp
{
  [super setUp];

  self.sessionManager = [[FakeRecorderAudioSessionManager alloc] init];
  self.audioEngine = [[FakeRecorderAudioEngine alloc] init];
  self.sharedSession = [[FakeRecorderSharedAVAudioSession alloc] init];
  self.sharedSession.IOBufferDuration = 0.01;
  self.sharedSession.sampleRate = 48000;
  SetFakeRecorderSharedAudioSession(self.sharedSession);

  self.audioEngine.fakeAVAudioEngine.fakeInputNode.format = [self validFormat];
  self.sessionManager.preferredInputFormat =
      [[AVAudioFormat alloc] initStandardFormatWithSampleRate:32000 channels:1];
}

- (void)tearDown
{
  [self.audioEngine cleanup];
  [self.sessionManager cleanup];
  ClearFakeRecorderSharedAudioSession();

  self.sharedSession = nil;
  self.audioEngine = nil;
  self.sessionManager = nil;

  [super tearDown];
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

- (void)testInitCreatesSinkNodeAndForwardsReceiverBlock
{
  __block const AudioBufferList *receivedBuffer = nullptr;
  __block int receivedFrames = 0;
  NativeAudioRecorder *recorder = [[NativeAudioRecorder alloc] initWithReceiverBlock:^(
      const AudioBufferList *inputBuffer,
      int numFrames) {
    receivedBuffer = inputBuffer;
    receivedFrames = numFrames;
  }];

  XCTAssertNotNil(recorder.sinkNode);
  XCTAssertNotNil(recorder.receiverBlock);
  XCTAssertNotNil(recorder.receiverSinkBlock);

  TestAudioInput input(1, 8);
  AudioTimeStamp timestamp = {};
  OSStatus status = recorder.receiverSinkBlock(&timestamp, 8, input.bufferList());

  XCTAssertEqual(status, (OSStatus)kAudioServicesNoError);
  XCTAssertEqual(receivedBuffer, input.bufferList());
  XCTAssertEqual(receivedFrames, 8);
}

- (void)testGetInputFormatReturnsEngineInputFormatWhenUsable
{
  AVAudioFormat *expectedFormat = [self validFormat];
  self.audioEngine.fakeAVAudioEngine.fakeInputNode.format = expectedFormat;

  NativeAudioRecorder *recorder =
      [[NativeAudioRecorder alloc] initWithReceiverBlock:^(const AudioBufferList *inputBuffer,
                                                           int numFrames){
      }];

  XCTAssertEqualObjects([recorder getInputFormat], expectedFormat);
}

- (void)testGetInputFormatFallsBackToSessionPreferredFormatWhenEngineInputIsInvalid
{
  self.audioEngine.fakeAVAudioEngine.fakeInputNode.format = [self invalidFormat];
  AVAudioFormat *fallbackFormat = self.sessionManager.preferredInputFormat;

  NativeAudioRecorder *recorder =
      [[NativeAudioRecorder alloc] initWithReceiverBlock:^(const AudioBufferList *inputBuffer,
                                                           int numFrames){
      }];

  XCTAssertEqualObjects([recorder getInputFormat], fallbackFormat);
}

- (void)testGetBufferSizeUsesMinimumDurationAndRoundsUpToPowerOfTwo
{
  NativeAudioRecorder *recorder =
      [[NativeAudioRecorder alloc] initWithReceiverBlock:^(const AudioBufferList *inputBuffer,
                                                           int numFrames){
      }];

  int bufferSize = [recorder getBufferSize];
  XCTAssertEqual(bufferSize, 16384);
  XCTAssertEqual(bufferSize & (bufferSize - 1), 0);
}

- (void)testStartStopsEngineAttachesSinkNodeAndStartsEngine
{
  AVAudioFormat *expectedFormat = [self validFormat];
  self.audioEngine.fakeAVAudioEngine.fakeInputNode.format = expectedFormat;
  NativeAudioRecorder *recorder =
      [[NativeAudioRecorder alloc] initWithReceiverBlock:^(const AudioBufferList *inputBuffer,
                                                           int numFrames){
      }];

  [recorder start];

  XCTAssertEqual(self.audioEngine.stopIfNecessaryCallCount, 1);
  XCTAssertEqual(self.audioEngine.attachInputNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 1);
  XCTAssertEqual(self.audioEngine.lastAttachedInputNode, recorder.sinkNode);
  XCTAssertEqualObjects(self.audioEngine.lastAttachedFormat, expectedFormat);
}

- (void)testStopRestartsEngineWhenItIsNotRunning
{
  self.audioEngine.state = AudioEngineStatePaused;
  NativeAudioRecorder *recorder =
      [[NativeAudioRecorder alloc] initWithReceiverBlock:^(const AudioBufferList *inputBuffer,
                                                           int numFrames){
      }];

  [recorder stop];

  XCTAssertEqual(self.audioEngine.detachInputNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.stopIfPossibleCallCount, 1);
  XCTAssertEqual(self.audioEngine.restartAudioEngineCallCount, 1);
}

- (void)testStopDoesNotRestartEngineWhenItIsAlreadyRunning
{
  self.audioEngine.state = AudioEngineStateRunning;
  NativeAudioRecorder *recorder =
      [[NativeAudioRecorder alloc] initWithReceiverBlock:^(const AudioBufferList *inputBuffer,
                                                           int numFrames){
      }];

  [recorder stop];

  XCTAssertEqual(self.audioEngine.detachInputNodeCallCount, 1);
  XCTAssertEqual(self.audioEngine.stopIfPossibleCallCount, 1);
  XCTAssertEqual(self.audioEngine.restartAudioEngineCallCount, 0);
}

- (void)testPauseAndResumeDelegateToAudioEngine
{
  NativeAudioRecorder *recorder =
      [[NativeAudioRecorder alloc] initWithReceiverBlock:^(const AudioBufferList *inputBuffer,
                                                           int numFrames){
      }];

  [recorder pause];
  [recorder resume];

  XCTAssertEqual(self.audioEngine.pauseIfNecessaryCallCount, 1);
  XCTAssertEqual(self.audioEngine.startIfNecessaryCallCount, 1);
}

- (void)testCleanupClearsReceiverBlock
{
  NativeAudioRecorder *recorder =
      [[NativeAudioRecorder alloc] initWithReceiverBlock:^(const AudioBufferList *inputBuffer,
                                                           int numFrames){
      }];

  [recorder cleanup];

  XCTAssertNil(recorder.receiverBlock);
}

@end

#import <XCTest/XCTest.h>

#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@interface AudioEngine (TestingPrivate)

- (void)createAudioEngineIfNeeded;

@end

@interface FakeAudioInputNode : NSObject

@property(nonatomic, strong) AVAudioFormat *outputFormat;

- (instancetype)initWithOutputFormat:(AVAudioFormat *)outputFormat;
- (AVAudioFormat *)outputFormatForBus:(AVAudioNodeBus)bus;

@end

@implementation FakeAudioInputNode

- (instancetype)initWithOutputFormat:(AVAudioFormat *)outputFormat {
  if (self = [super init]) {
    self.outputFormat = outputFormat;
  }

  return self;
}

- (AVAudioFormat *)outputFormatForBus:(AVAudioNodeBus)bus {
  return self.outputFormat;
}

@end

@interface FakeAudioEngine : AVAudioEngine

@property(nonatomic, assign) BOOL fakeRunning;
@property(nonatomic, strong) NSError *startError;
@property(nonatomic, assign) NSInteger attachNodeCallCount;
@property(nonatomic, assign) NSInteger detachNodeCallCount;
@property(nonatomic, assign) NSInteger connectCallCount;
@property(nonatomic, assign) NSInteger prepareCallCount;
@property(nonatomic, assign) NSInteger startCallCount;
@property(nonatomic, assign) NSInteger stopCallCount;
@property(nonatomic, assign) NSInteger pauseCallCount;
@property(nonatomic, assign) NSInteger resetCallCount;
@property(nonatomic, strong)
    NSMutableArray<AVAudioNode *> *recordedAttachedNodes;
@property(nonatomic, strong)
    NSMutableArray<AVAudioNode *> *recordedDetachedNodes;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *connections;
@property(nonatomic, strong) AVAudioNode *fakeMainMixerNode;
@property(nonatomic, strong) FakeAudioInputNode *fakeInputNode;

- (instancetype)initWithInputFormat:(AVAudioFormat *)inputFormat;

@end

@implementation FakeAudioEngine

- (instancetype)init {
  return [self initWithInputFormat:nil];
}

- (instancetype)initWithInputFormat:(AVAudioFormat *)inputFormat {
  if (self = [super init]) {
    self.recordedAttachedNodes = [[NSMutableArray alloc] init];
    self.recordedDetachedNodes = [[NSMutableArray alloc] init];
    self.connections = [[NSMutableArray alloc] init];
    self.fakeMainMixerNode = [[AVAudioMixerNode alloc] init];
    self.fakeInputNode =
        [[FakeAudioInputNode alloc] initWithOutputFormat:inputFormat];
  }

  return self;
}

- (BOOL)isRunning {
  return self.fakeRunning;
}

- (AVAudioMixerNode *)mainMixerNode {
  return (AVAudioMixerNode *)self.fakeMainMixerNode;
}

- (AVAudioInputNode *)inputNode {
  return (AVAudioInputNode *)self.fakeInputNode;
}

- (void)attachNode:(AVAudioNode *)node {
  self.attachNodeCallCount += 1;
  [self.recordedAttachedNodes addObject:node];
}

- (void)detachNode:(AVAudioNode *)node {
  self.detachNodeCallCount += 1;
  [self.recordedDetachedNodes addObject:node];
}

- (void)connect:(AVAudioNode *)node1
             to:(AVAudioNode *)node2
         format:(AVAudioFormat *)format {
  self.connectCallCount += 1;
  [self.connections addObject:@{
    @"from" : node1,
    @"to" : node2,
    @"format" : format ?: [NSNull null],
  }];
}

- (void)prepare {
  self.prepareCallCount += 1;
}

- (BOOL)startAndReturnError:(NSError **)outError {
  self.startCallCount += 1;

  if (outError != nil) {
    *outError = self.startError;
  }

  self.fakeRunning = self.startError == nil;
  return self.startError == nil;
}

- (void)stop {
  self.stopCallCount += 1;
  self.fakeRunning = NO;
}

- (void)pause {
  self.pauseCallCount += 1;
  self.fakeRunning = NO;
}

- (void)reset {
  self.resetCallCount += 1;
}

@end

@interface FakeAudioSessionManager : AudioSessionManager

@property(nonatomic, assign) BOOL ensureActiveResult;
@property(nonatomic, strong) NSError *ensureActiveFailure;
@property(nonatomic, assign) NSInteger ensureActiveCallCount;
@property(nonatomic, assign) NSInteger setActiveCallCount;
@property(nonatomic, assign) BOOL lastEnsureActiveForce;
@property(nonatomic, assign) BOOL lastSetActiveValue;

@end

@implementation FakeAudioSessionManager

- (instancetype)init {
  if (self = [super init]) {
    self.ensureActiveResult = YES;
  }

  return self;
}

- (bool)ensureActive:(bool)force error:(NSError **)error {
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

- (bool)setActive:(bool)active error:(NSError **)error {
  self.setActiveCallCount += 1;
  self.lastSetActiveValue = active;

  if (error != nil) {
    *error = nil;
  }

  self.isActive = active;
  return true;
}

@end

@interface TestableAudioEngine : AudioEngine

@property(nonatomic, strong)
    NSMutableArray<FakeAudioEngine *> *createdFakeEngines;
@property(nonatomic, strong) NSError *nextCreatedEngineStartError;
@property(nonatomic, strong) AVAudioFormat *defaultCreatedEngineInputFormat;
@property(nonatomic, strong) AVAudioFormat *nextCreatedEngineInputFormat;

- (FakeAudioEngine *)currentFakeAudioEngine;

@end

@implementation TestableAudioEngine

- (instancetype)init {
  if (self = [super init]) {
    if (self.createdFakeEngines == nil) {
      self.createdFakeEngines = [[NSMutableArray alloc] init];
    }
  }

  return self;
}

- (void)createAudioEngineIfNeeded {
  if (self.audioEngine != nil) {
    return;
  }

  if (self.createdFakeEngines == nil) {
    self.createdFakeEngines = [[NSMutableArray alloc] init];
  }

  AVAudioFormat *inputFormat =
      self.nextCreatedEngineInputFormat ?: self.defaultCreatedEngineInputFormat;
  self.nextCreatedEngineInputFormat = nil;

  FakeAudioEngine *engine =
      [[FakeAudioEngine alloc] initWithInputFormat:inputFormat];
  engine.startError = self.nextCreatedEngineStartError;
  self.nextCreatedEngineStartError = nil;
  [self.createdFakeEngines addObject:engine];
  self.audioEngine = engine;
}

- (FakeAudioEngine *)currentFakeAudioEngine {
  return (FakeAudioEngine *)self.audioEngine;
}

@end

@interface AudioEngineTests : XCTestCase

@property(nonatomic, strong) TestableAudioEngine *audioEngine;
@property(nonatomic, strong) FakeAudioSessionManager *sessionManager;

@end

@implementation AudioEngineTests

- (void)setUp {
  [super setUp];

  self.audioEngine = [[TestableAudioEngine alloc] init];
  AVAudioFormat *inputFormat = [self testInputFormat];
  self.audioEngine.defaultCreatedEngineInputFormat = inputFormat;
  self.audioEngine.currentFakeAudioEngine.fakeInputNode.outputFormat =
      inputFormat;
  self.sessionManager = [[FakeAudioSessionManager alloc] init];
  self.audioEngine.sessionManager = self.sessionManager;
}

- (void)tearDown {
  [self.audioEngine cleanup];
  self.audioEngine = nil;
  self.sessionManager = nil;

  [super tearDown];
}

- (AVAudioFormat *)testFormat {
  return [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100
                                                        channels:2];
}

- (AVAudioFormat *)testInputFormat {
  return [self testInputFormatWithSampleRate:44100 channelCount:1];
}

- (AVAudioFormat *)testInputFormatWithSampleRate:(double)sampleRate
                                    channelCount:
                                        (AVAudioChannelCount)channelCount {
  return [[AVAudioFormat alloc] initStandardFormatWithSampleRate:sampleRate
                                                        channels:channelCount];
}

- (AVAudioSourceNode *)testSourceNode {
  return [[AVAudioSourceNode alloc]
      initWithRenderBlock:^OSStatus(
          BOOL *isSilence, const AudioTimeStamp *timestamp,
          AVAudioFrameCount frameCount, AudioBufferList *outputData) {
        if (isSilence != nil) {
          *isSilence = YES;
        }

        return noErr;
      }];
}

- (AVAudioSourceNodeRenderBlock)testSourceRenderBlock {
  return ^OSStatus(BOOL *isSilence, const AudioTimeStamp *timestamp,
                   AVAudioFrameCount frameCount, AudioBufferList *outputData) {
    if (isSilence != nil) {
      *isSilence = YES;
    }

    return noErr;
  };
}

- (AVAudioSinkNodeReceiverBlock)testInputReceiverBlock {
  return ^OSStatus(const AudioTimeStamp *timestamp,
                   AVAudioFrameCount frameCount,
                   const AudioBufferList *inputData) {
    return noErr;
  };
}

- (NSString *)attachSourceNodeToAudioEngine {
  return [self.audioEngine
      attachSourceNodeWithRenderBlock:[self testSourceRenderBlock]
                           sampleRate:44100
                         channelCount:2];
}

- (void)testCleanupDestroysInternalEngineAndResetsStateAndDeactivatesSession {
  [self attachSourceNodeToAudioEngine];
  self.audioEngine.graphNeedsRebuild = YES;

  [self.audioEngine cleanup];

  XCTAssertNil(self.audioEngine.audioEngine);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertNil(self.audioEngine.sourceNodes);
  XCTAssertNil(self.audioEngine.sourceFormats);
  XCTAssertNil(self.audioEngine.inputNode);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
  XCTAssertFalse(self.audioEngine.sessionDeactivationInvalidatedGraph);
  XCTAssertEqual(self.sessionManager.setActiveCallCount, 1);
  XCTAssertFalse(self.sessionManager.lastSetActiveValue);
}

- (void)testGetStateAndIsEngineRunningReflectUnderlyingEngine {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  XCTAssertEqual([self.audioEngine getState], AudioEngineStateIdle);
  XCTAssertFalse([self.audioEngine isEngineRunning]);

  fakeEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  XCTAssertEqual([self.audioEngine getState], AudioEngineStateRunning);
  XCTAssertTrue([self.audioEngine isEngineRunning]);

  self.audioEngine.audioEngine = nil;
  XCTAssertFalse([self.audioEngine isEngineRunning]);
}

- (void)testAttachSourceNodeStoresAndConnectsSource {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  NSString *sourceNodeId = [self.audioEngine
      attachSourceNodeWithRenderBlock:[self testSourceRenderBlock]
                           sampleRate:44100
                         channelCount:2];
  AVAudioSourceNode *sourceNode = self.audioEngine.sourceNodes[sourceNodeId];
  AVAudioFormat *format = self.audioEngine.sourceFormats[sourceNodeId];

  XCTAssertNotNil(sourceNodeId);
  XCTAssertEqual(self.audioEngine.sourceNodes.count, 1UL);
  XCTAssertEqual(self.audioEngine.sourceFormats.count, 1UL);
  XCTAssertNotNil(sourceNode);
  XCTAssertNotNil(format);
  XCTAssertEqualWithAccuracy(format.sampleRate, 44100.0, 0.001);
  XCTAssertEqual(format.channelCount, 2U);
  XCTAssertEqual(fakeEngine.attachNodeCallCount, 1);
  XCTAssertEqual(fakeEngine.connectCallCount, 1);
  XCTAssertEqualObjects(fakeEngine.recordedAttachedNodes.firstObject,
                        sourceNode);
  XCTAssertEqualObjects(fakeEngine.connections.firstObject[@"from"],
                        sourceNode);
  XCTAssertEqualObjects(fakeEngine.connections.firstObject[@"to"],
                        fakeEngine.mainMixerNode);
  XCTAssertEqualObjects(fakeEngine.connections.firstObject[@"format"], format);
}

- (void)testDetachSourceNodeWithUnknownIdDoesNothing {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  [self.audioEngine detachSourceNodeWithId:@"missing-source"];

  XCTAssertEqual(fakeEngine.detachNodeCallCount, 0);
  XCTAssertEqual(self.audioEngine.sourceNodes.count, 0UL);
  XCTAssertEqual(self.audioEngine.sourceFormats.count, 0UL);
}

- (void)testDetachSourceNodeRemovesTrackedNodeAndClearsGraphWhenEmpty {
  NSString *sourceNodeId = [self attachSourceNodeToAudioEngine];
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  AVAudioSourceNode *sourceNode = self.audioEngine.sourceNodes[sourceNodeId];
  self.audioEngine.graphNeedsRebuild = YES;

  [self.audioEngine detachSourceNodeWithId:sourceNodeId];

  XCTAssertNil(self.audioEngine.sourceNodes[sourceNodeId]);
  XCTAssertNil(self.audioEngine.sourceFormats[sourceNodeId]);
  XCTAssertEqual(fakeEngine.detachNodeCallCount, 1);
  XCTAssertEqualObjects(fakeEngine.recordedDetachedNodes.firstObject,
                        sourceNode);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
}

- (void)testDetachSourceNodeKeepsGraphNeedsRebuildWhenInputRemains {
  NSString *sourceNodeId = [self attachSourceNodeToAudioEngine];
  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];
  self.audioEngine.graphNeedsRebuild = YES;

  [self.audioEngine detachSourceNodeWithId:sourceNodeId];

  XCTAssertEqual(self.audioEngine.sourceNodes.count, 0UL);
  XCTAssertNotNil(self.audioEngine.inputNode);
  XCTAssertTrue(self.audioEngine.graphNeedsRebuild);
}

- (void)testAttachInputNodeStoresAndConnectsInput {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];

  AVAudioSinkNode *inputNode = self.audioEngine.inputNode;
  XCTAssertNotNil(inputNode);
  XCTAssertEqual(fakeEngine.attachNodeCallCount, 1);
  XCTAssertEqual(fakeEngine.connectCallCount, 1);
  XCTAssertEqualObjects(fakeEngine.recordedAttachedNodes.firstObject,
                        inputNode);
  XCTAssertEqualObjects(fakeEngine.connections.firstObject[@"from"],
                        fakeEngine.inputNode);
  XCTAssertEqualObjects(fakeEngine.connections.firstObject[@"to"], inputNode);
  XCTAssertEqualObjects(fakeEngine.connections.firstObject[@"format"],
                        fakeEngine.fakeInputNode.outputFormat);
}

- (void)testAttachInputNodeDefersConnectionUntilLiveInputFormatIsAvailable {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeInputNode.outputFormat = nil;

  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];

  XCTAssertNil(self.audioEngine.inputNode);
  XCTAssertEqual(fakeEngine.attachNodeCallCount, 0);
  XCTAssertEqual(fakeEngine.connectCallCount, 0);

  AVAudioFormat *liveInputFormat = [self testInputFormat];
  fakeEngine.fakeInputNode.outputFormat = liveInputFormat;

  XCTAssertTrue([self.audioEngine startIfNecessary]);
  XCTAssertNotNil(self.audioEngine.inputNode);
  XCTAssertEqual(fakeEngine.attachNodeCallCount, 1);
  XCTAssertEqual(fakeEngine.connectCallCount, 1);
  XCTAssertEqualObjects(fakeEngine.connections.firstObject[@"format"],
                        liveInputFormat);
}

- (void)testDetachInputNodeWithoutInputDoesNothing {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  [self.audioEngine detachInputNode];

  XCTAssertNil(self.audioEngine.inputNode);
  XCTAssertEqual(fakeEngine.detachNodeCallCount, 0);
}

- (void)testDetachInputNodeClearsGraphOnlyWhenNoSourcesRemain {
  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];
  self.audioEngine.graphNeedsRebuild = YES;

  [self.audioEngine detachInputNode];

  XCTAssertNil(self.audioEngine.inputNode);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);

  [self attachSourceNodeToAudioEngine];
  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];
  self.audioEngine.graphNeedsRebuild = YES;

  [self.audioEngine detachInputNode];

  XCTAssertNil(self.audioEngine.inputNode);
  XCTAssertTrue(self.audioEngine.graphNeedsRebuild);
}

- (void)testDetachInputNodePreservesSessionDeactivationInvalidation {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;
  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];

  [self.audioEngine onSessionDeactivated];
  [self.audioEngine detachInputNode];

  XCTAssertNil(self.audioEngine.inputNode);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
  XCTAssertTrue(self.audioEngine.sessionDeactivationInvalidatedGraph);
}

- (void)testOnInterruptionBeginOnlyTransitionsFromRunning {
  [self.audioEngine onInterruptionBegin];
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);

  self.audioEngine.state = AudioEngineStatePaused;
  [self.audioEngine onInterruptionBegin];
  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);

  self.audioEngine.state = AudioEngineStateRunning;
  [self.audioEngine onInterruptionBegin];
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateInterrupted);
}

- (void)testOnSessionDeactivatedNoOpsFromIdle {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  [self.audioEngine onSessionDeactivated];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertEqual(fakeEngine.pauseCallCount, 0);
}

- (void)testOnSessionDeactivatedPausesRunningEngine {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  [self.audioEngine onSessionDeactivated];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);
  XCTAssertEqual(fakeEngine.pauseCallCount, 1);
  XCTAssertTrue(self.audioEngine.sessionDeactivationInvalidatedGraph);
}

- (void)testOnSessionDeactivatedMarksGraphForRebuildWhenNodesAreAttached {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;
  [self attachSourceNodeToAudioEngine];

  [self.audioEngine onSessionDeactivated];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);
  XCTAssertEqual(fakeEngine.pauseCallCount, 1);
  XCTAssertTrue(self.audioEngine.graphNeedsRebuild);
  XCTAssertTrue(self.audioEngine.sessionDeactivationInvalidatedGraph);
}

- (void)
    testOnSessionDeactivatedTransitionsToPausedWithoutPauseWhenEngineStopped {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeRunning = NO;
  self.audioEngine.state = AudioEngineStateRunning;

  [self.audioEngine onSessionDeactivated];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);
  XCTAssertEqual(fakeEngine.pauseCallCount, 0);
  XCTAssertTrue(self.audioEngine.sessionDeactivationInvalidatedGraph);
}

- (void)
    testOnSessionDeactivatedMarksStoppedGraphForRebuildWhenNodesAreAttached {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeRunning = NO;
  self.audioEngine.state = AudioEngineStateRunning;
  [self attachSourceNodeToAudioEngine];

  [self.audioEngine onSessionDeactivated];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);
  XCTAssertEqual(fakeEngine.pauseCallCount, 0);
  XCTAssertTrue(self.audioEngine.graphNeedsRebuild);
  XCTAssertTrue(self.audioEngine.sessionDeactivationInvalidatedGraph);
}

- (void)testOnInterruptionEndNoOpsUnlessInterrupted {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  [self.audioEngine onInterruptionEnd:true];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertEqual(fakeEngine.resetCallCount, 0);
  XCTAssertEqual(self.audioEngine.createdFakeEngines.count, 1UL);
}

- (void)testOnInterruptionEndWithoutResumeRebuildsAndPauses {
  [self attachSourceNodeToAudioEngine];

  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  oldEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateInterrupted;

  [self.audioEngine onInterruptionEnd:false];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);
  XCTAssertEqual(oldEngine.stopCallCount, 1);
  XCTAssertEqual(oldEngine.resetCallCount, 1);
  XCTAssertEqual(self.audioEngine.createdFakeEngines.count, 2UL);
  XCTAssertNotEqual(self.audioEngine.currentFakeAudioEngine, oldEngine);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
}

- (void)testOnInterruptionEndWithResumeRestartsEngine {
  [self attachSourceNodeToAudioEngine];

  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  oldEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateInterrupted;

  [self.audioEngine onInterruptionEnd:true];

  FakeAudioEngine *newEngine = self.audioEngine.currentFakeAudioEngine;
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateRunning);
  XCTAssertNotEqual(newEngine, oldEngine);
  XCTAssertEqual(newEngine.prepareCallCount, 1);
  XCTAssertEqual(newEngine.startCallCount, 1);
  XCTAssertEqual(self.audioEngine.createdFakeEngines.count, 2UL);
}

- (void)testOnInterruptionEndWithResumeFailureEndsIdle {
  [self attachSourceNodeToAudioEngine];

  self.audioEngine.state = AudioEngineStateInterrupted;
  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  oldEngine.fakeRunning = YES;
  self.audioEngine.nextCreatedEngineStartError =
      [NSError errorWithDomain:@"AudioEngineTests" code:5 userInfo:nil];

  [self.audioEngine onInterruptionEnd:true];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
}

- (void)testStartIfNecessaryReturnsFalseWhenGraphEmpty {
  XCTAssertFalse([self.audioEngine startIfNecessary]);
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 0);
}

- (void)testStartIfNecessaryReturnsTrueWhenAlreadyRunning {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  XCTAssertTrue([self.audioEngine startIfNecessary]);
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 0);
  XCTAssertEqual(fakeEngine.startCallCount, 0);
}

- (void)testStartIfNecessaryStartsWhenGraphExists {
  [self attachSourceNodeToAudioEngine];
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  XCTAssertTrue([self.audioEngine startIfNecessary]);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateRunning);
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 1);
  XCTAssertEqual(fakeEngine.prepareCallCount, 1);
  XCTAssertEqual(fakeEngine.startCallCount, 1);
}

- (void)testStartIfNecessaryReturnsFalseWhenSessionActivationFails {
  [self attachSourceNodeToAudioEngine];
  self.sessionManager.ensureActiveResult = NO;
  self.sessionManager.ensureActiveFailure =
      [NSError errorWithDomain:@"AudioEngineTests" code:7 userInfo:nil];
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  XCTAssertFalse([self.audioEngine startIfNecessary]);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 1);
  XCTAssertEqual(fakeEngine.startCallCount, 0);
}

- (void)testStartIfNecessaryRebuildsWhenInterrupted {
  [self attachSourceNodeToAudioEngine];
  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  oldEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateInterrupted;

  XCTAssertTrue([self.audioEngine startIfNecessary]);

  FakeAudioEngine *newEngine = self.audioEngine.currentFakeAudioEngine;
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateRunning);
  XCTAssertNotEqual(newEngine, oldEngine);
  XCTAssertEqual(self.audioEngine.createdFakeEngines.count, 2UL);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
  XCTAssertFalse(self.audioEngine.sessionDeactivationInvalidatedGraph);
}

- (void)testStartIfNecessaryRebuildsWhenGraphNeedsRebuild {
  [self attachSourceNodeToAudioEngine];
  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  AVAudioSourceNode *oldSourceNode =
      self.audioEngine.sourceNodes.allValues.firstObject;
  self.audioEngine.graphNeedsRebuild = YES;

  XCTAssertTrue([self.audioEngine startIfNecessary]);

  FakeAudioEngine *newEngine = self.audioEngine.currentFakeAudioEngine;
  AVAudioSourceNode *newSourceNode =
      self.audioEngine.sourceNodes.allValues.firstObject;
  XCTAssertNotEqual(newEngine, oldEngine);
  XCTAssertNotEqual(newSourceNode, oldSourceNode);
  XCTAssertEqual(self.audioEngine.createdFakeEngines.count, 2UL);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
  XCTAssertFalse(self.audioEngine.sessionDeactivationInvalidatedGraph);
  XCTAssertEqual(newEngine.prepareCallCount, 1);
  XCTAssertEqual(newEngine.startCallCount, 1);
}

- (void)
    testStartIfNecessaryRebuildsAfterSessionDeactivationEvenWhenTeardownClearsGraph {
  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];

  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  oldEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  [self.audioEngine onSessionDeactivated];
  [self.audioEngine detachInputNode];
  [self.audioEngine stopIfPossible];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
  XCTAssertTrue(self.audioEngine.sessionDeactivationInvalidatedGraph);

  AVAudioFormat *recoveredInputFormat =
      [self testInputFormatWithSampleRate:48000 channelCount:1];
  self.audioEngine.nextCreatedEngineInputFormat = recoveredInputFormat;
  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];
  AVAudioSinkNode *recoveredInputNode = self.audioEngine.inputNode;

  XCTAssertTrue([self.audioEngine startIfNecessary]);

  FakeAudioEngine *newEngine = self.audioEngine.currentFakeAudioEngine;
  XCTAssertNotEqual(newEngine, oldEngine);
  XCTAssertEqual(self.audioEngine.createdFakeEngines.count, 2UL);
  XCTAssertEqual(newEngine.connectCallCount, 1);
  XCTAssertEqualObjects(newEngine.connections.firstObject[@"from"],
                        newEngine.inputNode);
  XCTAssertEqualObjects(newEngine.connections.firstObject[@"to"],
                        self.audioEngine.inputNode);
  XCTAssertEqualObjects(newEngine.connections.firstObject[@"format"],
                        recoveredInputFormat);
  XCTAssertNotEqual(self.audioEngine.inputNode, recoveredInputNode);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
  XCTAssertFalse(self.audioEngine.sessionDeactivationInvalidatedGraph);
}

- (void)testStartIfNecessaryRebuildsInputNodeWithFreshInstance {
  [self.audioEngine
      attachInputNodeWithReceiverBlock:[self testInputReceiverBlock]];
  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  AVAudioSinkNode *oldInputNode = self.audioEngine.inputNode;
  AVAudioFormat *replacementInputFormat =
      [self testInputFormatWithSampleRate:48000 channelCount:1];
  self.audioEngine.nextCreatedEngineInputFormat = replacementInputFormat;
  self.audioEngine.graphNeedsRebuild = YES;

  XCTAssertTrue([self.audioEngine startIfNecessary]);

  FakeAudioEngine *newEngine = self.audioEngine.currentFakeAudioEngine;
  XCTAssertNotEqual(newEngine, oldEngine);
  XCTAssertNotEqual(self.audioEngine.inputNode, oldInputNode);
  XCTAssertEqual(newEngine.connectCallCount, 1);
  XCTAssertEqualObjects(newEngine.connections.firstObject[@"from"],
                        newEngine.inputNode);
  XCTAssertEqualObjects(newEngine.connections.firstObject[@"to"],
                        self.audioEngine.inputNode);
  XCTAssertEqualObjects(newEngine.connections.firstObject[@"format"],
                        replacementInputFormat);
}

- (void)testStartIfNecessaryReturnsFalseWhenEngineStartFails {
  [self attachSourceNodeToAudioEngine];
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.startError = [NSError errorWithDomain:@"AudioEngineTests"
                                              code:9
                                          userInfo:nil];

  XCTAssertFalse([self.audioEngine startIfNecessary]);
  XCTAssertEqual(fakeEngine.startCallCount, 1);
  XCTAssertNotEqual(self.audioEngine.state, AudioEngineStateRunning);
}

- (void)testPauseIfNecessaryNoOpsWhenAlreadyPaused {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  self.audioEngine.state = AudioEngineStatePaused;

  [self.audioEngine pauseIfNecessary];

  XCTAssertEqual(fakeEngine.pauseCallCount, 0);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);
}

- (void)testPauseIfNecessaryPausesActiveEngine {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  self.audioEngine.state = AudioEngineStateRunning;

  [self.audioEngine pauseIfNecessary];

  XCTAssertEqual(fakeEngine.pauseCallCount, 1);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);
}

- (void)testStopIfNecessaryNoOpsFromIdle {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  [self.audioEngine stopIfNecessary];

  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertEqual(fakeEngine.stopCallCount, 0);
  XCTAssertNotNil(self.audioEngine.audioEngine);
}

- (void)testStopIfNecessaryStopsButPreservesEngineInstance {
  [self attachSourceNodeToAudioEngine];
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  [self.audioEngine stopIfNecessary];

  XCTAssertEqual(self.audioEngine.audioEngine, fakeEngine);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertEqual(fakeEngine.stopCallCount, 1);
  XCTAssertEqual(fakeEngine.resetCallCount, 0);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
}

- (void)testStopIfPossibleNoOpsWhenGraphStillExists {
  [self attachSourceNodeToAudioEngine];
  self.audioEngine.state = AudioEngineStateRunning;
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  [self.audioEngine stopIfPossible];

  XCTAssertEqual(self.audioEngine.audioEngine, fakeEngine);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateRunning);
  XCTAssertEqual(fakeEngine.stopCallCount, 0);
}

- (void)testStopIfPossibleKeepsIdleGraphlessEngineAlive {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;

  [self.audioEngine stopIfPossible];

  XCTAssertEqual(self.audioEngine.audioEngine, fakeEngine);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertEqual(fakeEngine.stopCallCount, 0);
  XCTAssertEqual(fakeEngine.resetCallCount, 0);
}

- (void)testStopIfPossibleStopsNonIdleGraphlessEngineWithoutDestroyingIt {
  FakeAudioEngine *fakeEngine = self.audioEngine.currentFakeAudioEngine;
  fakeEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  [self.audioEngine stopIfPossible];

  XCTAssertEqual(self.audioEngine.audioEngine, fakeEngine);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateIdle);
  XCTAssertEqual(fakeEngine.stopCallCount, 1);
  XCTAssertEqual(fakeEngine.resetCallCount, 0);
}

- (void)testRestartAudioEngineRebuildsWithoutRestartWhenStateIsNotRunning {
  [self attachSourceNodeToAudioEngine];
  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  self.audioEngine.state = AudioEngineStatePaused;

  [self.audioEngine restartAudioEngine];

  FakeAudioEngine *newEngine = self.audioEngine.currentFakeAudioEngine;
  XCTAssertNotEqual(newEngine, oldEngine);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStatePaused);
  XCTAssertEqual(newEngine.startCallCount, 0);
  XCTAssertFalse(self.audioEngine.graphNeedsRebuild);
}

- (void)testRestartAudioEngineStopsAndRestartsWhenStateRunning {
  [self attachSourceNodeToAudioEngine];
  FakeAudioEngine *oldEngine = self.audioEngine.currentFakeAudioEngine;
  oldEngine.fakeRunning = YES;
  self.audioEngine.state = AudioEngineStateRunning;

  [self.audioEngine restartAudioEngine];

  FakeAudioEngine *newEngine = self.audioEngine.currentFakeAudioEngine;
  XCTAssertNotEqual(newEngine, oldEngine);
  XCTAssertEqual(oldEngine.stopCallCount, 1);
  XCTAssertEqual(newEngine.prepareCallCount, 1);
  XCTAssertEqual(newEngine.startCallCount, 1);
  XCTAssertEqual(self.sessionManager.ensureActiveCallCount, 1);
  XCTAssertEqual(self.audioEngine.state, AudioEngineStateRunning);
}

@end

#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import <audioapi/events/AudioEvent.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/SystemNotificationManager.h>

@interface SystemNotificationManager (TestingPrivate)

- (void)handleInterruption:(NSNotification *)notification;
- (void)handleSecondaryAudio:(NSNotification *)notification;
- (void)handleRouteChange:(NSNotification *)notification;
- (void)handleMediaServicesReset:(NSNotification *)notification;
- (void)handleEngineConfigurationChange:(NSNotification *)notification;
- (void)startPollingSecondaryAudioHint;
- (void)stopPollingSecondaryAudioHint;
- (void)checkSecondaryAudioHint;

@end

@interface FakeNotificationCenter : NSObject

@property(nonatomic, assign) NSInteger addObserverCallCount;
@property(nonatomic, assign) NSInteger removeObserverCallCount;
@property(nonatomic, strong) NSMutableArray<id> *addedNotificationNames;
@property(nonatomic, strong) NSMutableArray<id> *removedNotificationNames;

@end

@implementation FakeNotificationCenter

- (instancetype)init
{
  if (self = [super init]) {
    self.addedNotificationNames = [[NSMutableArray alloc] init];
    self.removedNotificationNames = [[NSMutableArray alloc] init];
  }

  return self;
}

- (void)addObserver:(id)observer
           selector:(SEL)aSelector
               name:(NSNotificationName)aName
             object:(id)anObject
{
  self.addObserverCallCount += 1;
  [self.addedNotificationNames addObject:aName ?: [NSNull null]];
}

- (void)removeObserver:(id)observer name:(NSNotificationName)aName object:(id)anObject
{
  self.removeObserverCallCount += 1;
  [self.removedNotificationNames addObject:aName ?: [NSNull null]];
}

@end

@interface FakeSharedAVAudioSession : NSObject

@property(nonatomic, assign) BOOL secondaryAudioShouldBeSilencedHint;
@property(nonatomic, assign) float outputVolume;
@property(nonatomic, assign) NSInteger addObserverCallCount;
@property(nonatomic, assign) NSInteger removeObserverCallCount;
@property(nonatomic, copy) NSString *lastAddedKeyPath;
@property(nonatomic, assign) NSKeyValueObservingOptions lastAddedOptions;
@property(nonatomic, assign) void *lastAddedContext;

@end

@implementation FakeSharedAVAudioSession

- (void)addObserver:(NSObject *)observer
         forKeyPath:(NSString *)keyPath
            options:(NSKeyValueObservingOptions)options
            context:(void *)context
{
  self.addObserverCallCount += 1;
  self.lastAddedKeyPath = keyPath;
  self.lastAddedOptions = options;
  self.lastAddedContext = context;
}

- (void)removeObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath context:(void *)context
{
  self.removeObserverCallCount += 1;
}

@end

@interface SNMFakeAudioEngine : AudioEngine

@property(nonatomic, assign) NSInteger interruptionBeginCallCount;
@property(nonatomic, assign) NSInteger interruptionEndCallCount;
@property(nonatomic, assign) NSInteger restartAudioEngineCallCount;
@property(nonatomic, assign) BOOL lastShouldResume;

@end

@implementation SNMFakeAudioEngine

- (void)onInterruptionBegin
{
  self.interruptionBeginCallCount += 1;
}

- (void)onInterruptionEnd:(bool)shouldResume
{
  self.interruptionEndCallCount += 1;
  self.lastShouldResume = shouldResume;
}

- (void)restartAudioEngine
{
  self.restartAudioEngineCallCount += 1;
}

@end

@interface SNMFakeAudioSessionManager : AudioSessionManager

@property(nonatomic, assign) NSInteger markInactiveCallCount;
@property(nonatomic, assign) NSInteger ensureActiveCallCount;
@property(nonatomic, assign) BOOL lastEnsureActiveForce;

@end

@implementation SNMFakeAudioSessionManager

- (void)markInactive
{
  self.markInactiveCallCount += 1;
  self.isActive = false;
}

- (bool)ensureActive:(bool)force error:(NSError **)error
{
  self.ensureActiveCallCount += 1;
  self.lastEnsureActiveForce = force;

  if (error != nil) {
    *error = nil;
  }

  self.isActive = true;
  return true;
}

@end

@interface FakeAudioAPIModule : NSObject

@property(nonatomic, strong) AudioEngine *audioEngine;
@property(nonatomic, strong) AudioSessionManager *audioSessionManager;
@property(nonatomic, assign) NSInteger eventInvocationCount;
@property(nonatomic, assign) NSInteger lastEventNameRaw;
@property(nonatomic, strong) NSDictionary *lastEventBody;

- (void)resetCapturedEvent;
- (void)invokeHandlerWithEventName:(audioapi::AudioEvent)eventName eventBody:(NSDictionary *)eventBody;

@end

@implementation FakeAudioAPIModule

- (void)resetCapturedEvent
{
  self.eventInvocationCount = 0;
  self.lastEventNameRaw = -1;
  self.lastEventBody = nil;
}

- (void)invokeHandlerWithEventName:(audioapi::AudioEvent)eventName eventBody:(NSDictionary *)eventBody
{
  self.eventInvocationCount += 1;
  self.lastEventNameRaw = static_cast<NSInteger>(eventName);
  self.lastEventBody = eventBody;
}

@end

static id gFakeSharedAudioSession = nil;
static BOOL gSharedAudioSessionSwizzled = NO;

@interface AVAudioSession (SystemNotificationManagerTests)

+ (id)rna_test_sharedInstance;

@end

@implementation AVAudioSession (SystemNotificationManagerTests)

+ (id)rna_test_sharedInstance
{
  if (gFakeSharedAudioSession != nil) {
    return gFakeSharedAudioSession;
  }

  return [self rna_test_sharedInstance];
}

@end

static void SetFakeSharedAudioSession(id fakeSharedAudioSession)
{
  Method originalMethod = class_getClassMethod([AVAudioSession class], @selector(sharedInstance));
  Method swizzledMethod =
      class_getClassMethod([AVAudioSession class], @selector(rna_test_sharedInstance));

  if (!gSharedAudioSessionSwizzled) {
    method_exchangeImplementations(originalMethod, swizzledMethod);
    gSharedAudioSessionSwizzled = YES;
  }

  gFakeSharedAudioSession = fakeSharedAudioSession;
}

static void ClearFakeSharedAudioSession(void)
{
  if (gSharedAudioSessionSwizzled) {
    Method originalMethod = class_getClassMethod([AVAudioSession class], @selector(sharedInstance));
    Method swizzledMethod =
        class_getClassMethod([AVAudioSession class], @selector(rna_test_sharedInstance));
    method_exchangeImplementations(originalMethod, swizzledMethod);
    gSharedAudioSessionSwizzled = NO;
  }

  gFakeSharedAudioSession = nil;
}

@interface SystemNotificationManagerTests : XCTestCase

@property(nonatomic, strong) SystemNotificationManager *manager;
@property(nonatomic, strong) FakeAudioAPIModule *module;
@property(nonatomic, strong) SNMFakeAudioEngine *fakeAudioEngine;
@property(nonatomic, strong) SNMFakeAudioSessionManager *fakeSessionManager;
@property(nonatomic, strong) FakeSharedAVAudioSession *fakeSharedAudioSession;

@end

@implementation SystemNotificationManagerTests

- (void)setUp
{
  [super setUp];

  self.fakeAudioEngine = [[SNMFakeAudioEngine alloc] init];
  self.fakeSessionManager = [[SNMFakeAudioSessionManager alloc] init];
  self.fakeSharedAudioSession = [[FakeSharedAVAudioSession alloc] init];
  SetFakeSharedAudioSession(self.fakeSharedAudioSession);

  self.module = [[FakeAudioAPIModule alloc] init];
  self.module.audioEngine = self.fakeAudioEngine;
  self.module.audioSessionManager = self.fakeSessionManager;
  [self.module resetCapturedEvent];

  self.manager = [[SystemNotificationManager alloc] initWithAudioAPIModule:(id)self.module];
}

- (void)tearDown
{
  [self.manager stopPollingSecondaryAudioHint];
  [self.manager cleanup];
  self.manager = nil;
  self.module = nil;
  self.fakeAudioEngine = nil;
  self.fakeSessionManager = nil;
  self.fakeSharedAudioSession = nil;

  ClearFakeSharedAudioSession();

  [super tearDown];
}

- (void)flushMainQueue
{
  XCTestExpectation *expectation = [self expectationWithDescription:@"Flush main queue"];
  dispatch_async(dispatch_get_main_queue(), ^{ [expectation fulfill]; });
  [self waitForExpectations:@[ expectation ] timeout:1.0];
}

- (NSNotification *)interruptionNotificationWithType:(AVAudioSessionInterruptionType)type
                                              option:(AVAudioSessionInterruptionOptions)option
{
  return [NSNotification notificationWithName:AVAudioSessionInterruptionNotification
                                       object:nil
                                     userInfo:@{
                                       AVAudioSessionInterruptionTypeKey : @(type),
                                       AVAudioSessionInterruptionOptionKey : @(option),
                                     }];
}

- (NSNotification *)secondaryAudioNotificationWithType:(AVAudioSessionSilenceSecondaryAudioHintType)type
{
  return [NSNotification
      notificationWithName:AVAudioSessionSilenceSecondaryAudioHintNotification
                    object:nil
                  userInfo:@{
                    AVAudioSessionSilenceSecondaryAudioHintTypeKey : @(type),
                  }];
}

- (NSNotification *)routeChangeNotificationWithReason:(AVAudioSessionRouteChangeReason)reason
{
  return [NSNotification notificationWithName:AVAudioSessionRouteChangeNotification
                                       object:nil
                                     userInfo:@{
                                       AVAudioSessionRouteChangeReasonKey : @(reason),
                                     }];
}

- (void)testObserveVolumeChangesIsIdempotent
{
  [self.manager observeVolumeChanges:YES];
  [self.manager observeVolumeChanges:YES];

  XCTAssertTrue(self.manager.volumeChangesObserved);
  XCTAssertEqual(self.fakeSharedAudioSession.addObserverCallCount, 1);
  XCTAssertEqualObjects(self.fakeSharedAudioSession.lastAddedKeyPath, @"outputVolume");
  XCTAssertEqual(self.fakeSharedAudioSession.lastAddedOptions, NSKeyValueObservingOptionNew);
  XCTAssertNotEqual(self.fakeSharedAudioSession.lastAddedContext, nullptr);

  [self.manager observeVolumeChanges:NO];
  [self.manager observeVolumeChanges:NO];

  XCTAssertFalse(self.manager.volumeChangesObserved);
  XCTAssertEqual(self.fakeSharedAudioSession.removeObserverCallCount, 1);
}

- (void)testObserveValueForKeyPathRequiresExpectedContextAndEnabledObservation
{
  [self.manager observeVolumeChanges:YES];

  [self.module resetCapturedEvent];
  [self.manager observeValueForKeyPath:@"outputVolume"
                              ofObject:nil
                                change:@{@"new" : @0.25f}
                               context:nil];
  XCTAssertEqual(self.module.eventInvocationCount, 0);

  [self.manager observeValueForKeyPath:@"outputVolume"
                              ofObject:nil
                                change:@{@"new" : @0.75f}
                               context:self.fakeSharedAudioSession.lastAddedContext];

  XCTAssertEqual(self.module.eventInvocationCount, 1);
  XCTAssertEqual(self.module.lastEventNameRaw,
                 static_cast<NSInteger>(audioapi::AudioEvent::VOLUME_CHANGE));
  XCTAssertEqualObjects(self.module.lastEventBody[@"value"], @0.75f);
}

- (void)testActivelyReclaimSessionRegistersObserverAndStartsPolling
{
  FakeNotificationCenter *fakeNotificationCenter = [[FakeNotificationCenter alloc] init];
  self.manager.notificationCenter = (NSNotificationCenter *)fakeNotificationCenter;

  [self.manager activelyReclaimSession:YES];
  [self flushMainQueue];

  XCTAssertEqual(fakeNotificationCenter.addObserverCallCount, 1);
  XCTAssertEqualObjects(fakeNotificationCenter.addedNotificationNames.firstObject,
                        AVAudioSessionSilenceSecondaryAudioHintNotification);
  XCTAssertNotNil(self.manager.hintPollingTimer);

  [self.manager activelyReclaimSession:NO];

  XCTAssertEqual(fakeNotificationCenter.removeObserverCallCount, 1);
  XCTAssertEqualObjects(fakeNotificationCenter.removedNotificationNames.firstObject,
                        AVAudioSessionSilenceSecondaryAudioHintNotification);
  XCTAssertNil(self.manager.hintPollingTimer);
}

- (void)testHandleInterruptionBeganMarksInactiveAndEmitsEventWhenObserved
{
  [self.manager observeAudioInterruptions:YES];

  [self.manager handleInterruption:[self interruptionNotificationWithType:AVAudioSessionInterruptionTypeBegan
                                                                   option:0]];
  [self flushMainQueue];

  XCTAssertEqual(self.fakeSessionManager.markInactiveCallCount, 1);
  XCTAssertEqual(self.fakeAudioEngine.interruptionBeginCallCount, 1);
  XCTAssertEqual(self.module.eventInvocationCount, 1);
  XCTAssertEqual(self.module.lastEventNameRaw,
                 static_cast<NSInteger>(audioapi::AudioEvent::INTERRUPTION));
  XCTAssertEqualObjects(self.module.lastEventBody[@"type"], @"began");
  XCTAssertEqualObjects(self.module.lastEventBody[@"shouldResume"], @NO);
}

- (void)testHandleInterruptionEndedEmitsEventWhenObserved
{
  [self.manager observeAudioInterruptions:YES];

  [self.manager handleInterruption:[self interruptionNotificationWithType:AVAudioSessionInterruptionTypeEnded
                                                                   option:AVAudioSessionInterruptionOptionShouldResume]];

  XCTAssertEqual(self.module.eventInvocationCount, 1);
  XCTAssertEqual(self.module.lastEventNameRaw,
                 static_cast<NSInteger>(audioapi::AudioEvent::INTERRUPTION));
  XCTAssertEqualObjects(self.module.lastEventBody[@"type"], @"ended");
  XCTAssertEqualObjects(self.module.lastEventBody[@"shouldResume"], @YES);
  XCTAssertEqual(self.fakeAudioEngine.interruptionEndCallCount, 0);
}

- (void)testHandleInterruptionEndedResumesEngineWhenNotObserved
{
  [self.manager handleInterruption:[self interruptionNotificationWithType:AVAudioSessionInterruptionTypeEnded
                                                                   option:AVAudioSessionInterruptionOptionShouldResume]];
  [self flushMainQueue];

  XCTAssertEqual(self.module.eventInvocationCount, 0);
  XCTAssertEqual(self.fakeAudioEngine.interruptionEndCallCount, 1);
  XCTAssertTrue(self.fakeAudioEngine.lastShouldResume);
}

- (void)testHandleSecondaryAudioBeginMarksInactiveAndEmitsEventWhenObserved
{
  [self.manager observeAudioInterruptions:YES];

  [self.manager
      handleSecondaryAudio:[self secondaryAudioNotificationWithType:
                                     AVAudioSessionSilenceSecondaryAudioHintTypeBegin]];
  [self flushMainQueue];

  XCTAssertEqual(self.fakeSessionManager.markInactiveCallCount, 1);
  XCTAssertEqual(self.fakeAudioEngine.interruptionBeginCallCount, 1);
  XCTAssertEqual(self.module.eventInvocationCount, 1);
  XCTAssertEqual(self.module.lastEventNameRaw,
                 static_cast<NSInteger>(audioapi::AudioEvent::INTERRUPTION));
  XCTAssertEqualObjects(self.module.lastEventBody[@"type"], @"began");
  XCTAssertEqualObjects(self.module.lastEventBody[@"shouldResume"], @NO);
}

- (void)testHandleSecondaryAudioEndResumesEngineWhenNotObserved
{
  [self.manager
      handleSecondaryAudio:[self secondaryAudioNotificationWithType:
                                     AVAudioSessionSilenceSecondaryAudioHintTypeEnd]];
  [self flushMainQueue];

  XCTAssertEqual(self.module.eventInvocationCount, 0);
  XCTAssertEqual(self.fakeAudioEngine.interruptionEndCallCount, 1);
  XCTAssertTrue(self.fakeAudioEngine.lastShouldResume);
}

- (void)testHandleRouteChangeMapsReasonsAndFallsBackToUnknown
{
  NSArray<NSDictionary *> *cases = @[
    @{
      @"reason" : @(AVAudioSessionRouteChangeReasonNewDeviceAvailable),
      @"expected" : @"NewDeviceAvailable",
    },
    @{
      @"reason" : @(AVAudioSessionRouteChangeReasonOldDeviceUnavailable),
      @"expected" : @"OldDeviceUnavailable",
    },
    @{
      @"reason" : @(AVAudioSessionRouteChangeReasonRouteConfigurationChange),
      @"expected" : @"ConfigurationChange",
    },
    @{
      @"reason" : @999,
      @"expected" : @"Unknown",
    },
  ];

  for (NSDictionary *testCase in cases) {
    [self.module resetCapturedEvent];

    [self.manager handleRouteChange:[self routeChangeNotificationWithReason:
                                                  (AVAudioSessionRouteChangeReason)[testCase[@"reason"]
                                                      integerValue]]];

    XCTAssertEqual(self.module.eventInvocationCount, 1);
    XCTAssertEqual(self.module.lastEventNameRaw,
                   static_cast<NSInteger>(audioapi::AudioEvent::ROUTE_CHANGE));
    XCTAssertEqualObjects(self.module.lastEventBody[@"reason"], testCase[@"expected"]);
  }
}

- (void)testHandleMediaServicesResetReactivatesSessionAndRestartsEngine
{
  [self.manager handleMediaServicesReset:nil];
  [self flushMainQueue];

  XCTAssertEqual(self.fakeSessionManager.markInactiveCallCount, 1);
  XCTAssertEqual(self.fakeSessionManager.ensureActiveCallCount, 1);
  XCTAssertTrue(self.fakeSessionManager.lastEnsureActiveForce);
  XCTAssertEqual(self.fakeAudioEngine.restartAudioEngineCallCount, 1);
}

- (void)testHandleEngineConfigurationChangeMarksInactiveAndRestartsEngine
{
  [self.manager handleEngineConfigurationChange:nil];
  [self flushMainQueue];

  XCTAssertEqual(self.fakeSessionManager.markInactiveCallCount, 1);
  XCTAssertEqual(self.fakeAudioEngine.restartAudioEngineCallCount, 1);
}

- (void)testCheckSecondaryAudioHintDoesNothingWhenStateIsUnchanged
{
  self.fakeSharedAudioSession.secondaryAudioShouldBeSilencedHint = NO;
  self.manager.wasOtherAudioPlaying = NO;

  [self.manager checkSecondaryAudioHint];
  [self flushMainQueue];

  XCTAssertEqual(self.module.eventInvocationCount, 0);
  XCTAssertEqual(self.fakeSessionManager.markInactiveCallCount, 0);
  XCTAssertEqual(self.fakeAudioEngine.interruptionBeginCallCount, 0);
  XCTAssertEqual(self.fakeAudioEngine.interruptionEndCallCount, 0);
}

- (void)testCheckSecondaryAudioHintSilencedTransitionMarksInactiveAndEmitsEventWhenObserved
{
  [self.manager observeAudioInterruptions:YES];
  self.fakeSharedAudioSession.secondaryAudioShouldBeSilencedHint = YES;
  self.manager.wasOtherAudioPlaying = NO;

  [self.manager checkSecondaryAudioHint];
  [self flushMainQueue];

  XCTAssertTrue(self.manager.wasOtherAudioPlaying);
  XCTAssertEqual(self.fakeSessionManager.markInactiveCallCount, 1);
  XCTAssertEqual(self.fakeAudioEngine.interruptionBeginCallCount, 1);
  XCTAssertEqual(self.module.eventInvocationCount, 1);
  XCTAssertEqual(self.module.lastEventNameRaw,
                 static_cast<NSInteger>(audioapi::AudioEvent::INTERRUPTION));
  XCTAssertEqualObjects(self.module.lastEventBody[@"type"], @"began");
  XCTAssertEqualObjects(self.module.lastEventBody[@"shouldResume"], @NO);
}

- (void)testCheckSecondaryAudioHintResumeTransitionEmitsEventWhenObserved
{
  [self.manager observeAudioInterruptions:YES];
  self.fakeSharedAudioSession.secondaryAudioShouldBeSilencedHint = NO;
  self.manager.wasOtherAudioPlaying = YES;

  [self.manager checkSecondaryAudioHint];

  XCTAssertFalse(self.manager.wasOtherAudioPlaying);
  XCTAssertEqual(self.module.eventInvocationCount, 1);
  XCTAssertEqual(self.module.lastEventNameRaw,
                 static_cast<NSInteger>(audioapi::AudioEvent::INTERRUPTION));
  XCTAssertEqualObjects(self.module.lastEventBody[@"type"], @"ended");
  XCTAssertEqualObjects(self.module.lastEventBody[@"shouldResume"], @YES);
  XCTAssertEqual(self.fakeAudioEngine.interruptionEndCallCount, 0);
}

- (void)testCheckSecondaryAudioHintResumeTransitionResumesEngineWhenNotObserved
{
  self.fakeSharedAudioSession.secondaryAudioShouldBeSilencedHint = NO;
  self.manager.wasOtherAudioPlaying = YES;

  [self.manager checkSecondaryAudioHint];
  [self flushMainQueue];

  XCTAssertFalse(self.manager.wasOtherAudioPlaying);
  XCTAssertEqual(self.module.eventInvocationCount, 0);
  XCTAssertEqual(self.fakeAudioEngine.interruptionEndCallCount, 1);
  XCTAssertTrue(self.fakeAudioEngine.lastShouldResume);
}

@end

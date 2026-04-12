#import <XCTest/XCTest.h>

#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@interface AudioAPIModule (TestingPrivate)

- (void)setAudioSessionActivity:(BOOL)enabled
                        resolve:(RCTPromiseResolveBlock)resolve
                         reject:(RCTPromiseRejectBlock)reject;

@end

static void AppendAudioModuleEvent(NSMutableArray<NSString *> *eventLog, NSString *event)
{
  @synchronized(eventLog) {
    [eventLog addObject:event];
  }
}

static NSArray<NSString *> *CopyAudioModuleEvents(NSMutableArray<NSString *> *eventLog)
{
  @synchronized(eventLog) {
    return [eventLog copy];
  }
}

@interface FakeAudioAPIModuleEngine : AudioEngine

@property(nonatomic, assign) NSInteger onSessionDeactivatedCallCount;
@property(nonatomic, strong) NSMutableArray<NSString *> *eventLog;

@end

@implementation FakeAudioAPIModuleEngine

- (void)createAudioEngineIfNeeded
{
  // Avoid spinning up a real AVAudioEngine for module unit tests.
}

- (void)onSessionDeactivated
{
  self.onSessionDeactivatedCallCount += 1;
  AppendAudioModuleEvent(self.eventLog, @"onSessionDeactivated");
}

@end

@interface FakeAudioAPIModuleSessionManager : AudioSessionManager

@property(nonatomic, assign) NSInteger setActiveCallCount;
@property(nonatomic, assign) NSInteger markInactiveCallCount;
@property(nonatomic, assign) BOOL lastSetActiveValue;
@property(nonatomic, strong) NSMutableArray<NSString *> *eventLog;

@end

@implementation FakeAudioAPIModuleSessionManager

- (bool)setActive:(bool)active error:(NSError **)error
{
  self.setActiveCallCount += 1;
  self.lastSetActiveValue = active;
  self.isActive = active;
  AppendAudioModuleEvent(self.eventLog, @"setActive");

  if (error != nil) {
    *error = nil;
  }

  return true;
}

- (void)markInactive
{
  self.markInactiveCallCount += 1;
  self.isActive = false;
  AppendAudioModuleEvent(self.eventLog, @"markInactive");
}

@end

@interface AudioAPIModuleTests : XCTestCase

@property(nonatomic, strong) AudioAPIModule *module;
@property(nonatomic, strong) FakeAudioAPIModuleEngine *fakeAudioEngine;
@property(nonatomic, strong) FakeAudioAPIModuleSessionManager *fakeSessionManager;
@property(nonatomic, strong) NSMutableArray<NSString *> *eventLog;

@end

@implementation AudioAPIModuleTests

- (void)setUp
{
  [super setUp];

  self.eventLog = [[NSMutableArray alloc] init];
  self.module = [[AudioAPIModule alloc] init];
  self.fakeAudioEngine = [[FakeAudioAPIModuleEngine alloc] init];
  self.fakeAudioEngine.eventLog = self.eventLog;
  self.fakeSessionManager = [[FakeAudioAPIModuleSessionManager alloc] init];
  self.fakeSessionManager.eventLog = self.eventLog;
  self.module.audioEngine = self.fakeAudioEngine;
  self.module.audioSessionManager = self.fakeSessionManager;
}

- (void)tearDown
{
  [self.fakeAudioEngine cleanup];
  [self.fakeSessionManager cleanup];

  self.module = nil;
  self.fakeAudioEngine = nil;
  self.fakeSessionManager = nil;
  self.eventLog = nil;

  [super tearDown];
}

- (void)testSetAudioSessionActivityFalseWaitsForSessionDeactivationBeforeResolve
{
  XCTestExpectation *resolveExpectation = [self expectationWithDescription:@"setAudioSessionActivity"];
  __block NSArray<NSString *> *eventsAtResolve = nil;
  __block NSString *rejectionCode = nil;
  NSMutableArray<NSString *> *eventLog = self.eventLog;

  [self.module setAudioSessionActivity:NO
                               resolve:^(id result) {
                                 AppendAudioModuleEvent(eventLog, @"resolve");
                                 eventsAtResolve = CopyAudioModuleEvents(eventLog);
                                 XCTAssertEqualObjects(result, @YES);
                                 [resolveExpectation fulfill];
                               }
                                reject:^(NSString *code, NSString *message, NSError *error) {
                                  rejectionCode = code;
                                  [resolveExpectation fulfill];
                                }];

  [self waitForExpectations:@[ resolveExpectation ] timeout:1.0];

  XCTAssertNil(rejectionCode);
  XCTAssertEqual(self.fakeSessionManager.setActiveCallCount, 1);
  XCTAssertFalse(self.fakeSessionManager.lastSetActiveValue);
  XCTAssertEqual(self.fakeSessionManager.markInactiveCallCount, 1);
  XCTAssertEqual(self.fakeAudioEngine.onSessionDeactivatedCallCount, 1);
  XCTAssertFalse(self.fakeSessionManager.isActive);
  XCTAssertEqualObjects(
      eventsAtResolve,
      (@[ @"setActive", @"markInactive", @"onSessionDeactivated", @"resolve" ]));
}

@end

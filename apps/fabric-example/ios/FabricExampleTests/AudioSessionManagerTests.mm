#import <XCTest/XCTest.h>

#import <audioapi/ios/system/AudioSessionManager.h>

@interface AudioSessionManager (TestingPrivate)

- (AVAudioSessionCategory)categoryFromString:(NSString *)categorySTR;
- (AVAudioSessionMode)modeFromString:(NSString *)modeSTR;
- (AVAudioSessionCategoryOptions)optionsFromArray:(NSArray *)optionsArray;
- (bool)activateSessionIfNeeded:(bool)force error:(NSError **)error;
- (bool)areDesiredOptionsSet;
- (id)microphoneUsageDescriptionValue;
- (bool)usesAudioApplicationRecordPermissionAPI;
- (void)requestSystemRecordPermission:(void (^)(BOOL granted))completion;
- (NSInteger)currentRecordPermissionStatus;

@end

@interface FakeAudioPortDescription : NSObject

@property(nonatomic, copy) NSString *portName;
@property(nonatomic, copy) NSString *portType;
@property(nonatomic, copy) NSString *UID;

@end

@implementation FakeAudioPortDescription

@end

@interface FakeAudioRouteDescription : NSObject

@property(nonatomic, strong) NSArray<FakeAudioPortDescription *> *inputs;
@property(nonatomic, strong) NSArray<FakeAudioPortDescription *> *outputs;

@end

@implementation FakeAudioRouteDescription

@end

@interface FakeAVAudioSession : NSObject

@property(nonatomic, copy) AVAudioSessionCategory category;
@property(nonatomic, copy) AVAudioSessionMode mode;
@property(nonatomic, assign) AVAudioSessionCategoryOptions categoryOptions;
@property(nonatomic, assign) BOOL allowHapticsAndSystemSoundsDuringRecording;
@property(nonatomic, assign) double sampleRate;
@property(nonatomic, assign) NSInteger inputNumberOfChannels;
@property(nonatomic, strong) NSArray<FakeAudioPortDescription *> *availableInputs;
@property(nonatomic, strong) FakeAudioRouteDescription *currentRoute;
@property(nonatomic, assign) NSInteger recordPermission;
@property(nonatomic, strong) NSError *setCategoryError;
@property(nonatomic, strong) NSError *setAllowHapticsError;
@property(nonatomic, strong) NSError *setActiveError;
@property(nonatomic, strong) NSError *setPreferredInputError;
@property(nonatomic, assign) NSInteger setCategoryCallCount;
@property(nonatomic, assign) NSInteger setAllowHapticsCallCount;
@property(nonatomic, assign) NSInteger setActiveCallCount;
@property(nonatomic, assign) NSInteger setPreferredInputCallCount;
@property(nonatomic, assign) BOOL lastSetActiveValue;
@property(nonatomic, assign) AVAudioSessionSetActiveOptions lastSetActiveOptions;
@property(nonatomic, strong) FakeAudioPortDescription *lastPreferredInput;

@end

@implementation FakeAVAudioSession

- (instancetype)init
{
  if (self = [super init]) {
    self.category = AVAudioSessionCategoryPlayback;
    self.mode = AVAudioSessionModeDefault;
    self.categoryOptions = 0;
    self.allowHapticsAndSystemSoundsDuringRecording = NO;
    self.sampleRate = 44100;
    self.inputNumberOfChannels = 2;
    self.availableInputs = @[];
    self.currentRoute = [[FakeAudioRouteDescription alloc] init];
    self.currentRoute.inputs = @[];
    self.currentRoute.outputs = @[];
    self.recordPermission = AVAudioSessionRecordPermissionUndetermined;
  }

  return self;
}

- (BOOL)setCategory:(AVAudioSessionCategory)category
               mode:(AVAudioSessionMode)mode
            options:(AVAudioSessionCategoryOptions)options
              error:(NSError **)error
{
  self.setCategoryCallCount += 1;

  if (error != nil) {
    *error = self.setCategoryError;
  }

  if (self.setCategoryError != nil) {
    return NO;
  }

  self.category = category;
  self.mode = mode;
  self.categoryOptions = options;
  return YES;
}

- (BOOL)setAllowHapticsAndSystemSoundsDuringRecording:(BOOL)allowHapticsAndSystemSoundsDuringRecording
                                                error:(NSError **)error
{
  self.setAllowHapticsCallCount += 1;

  if (error != nil) {
    *error = self.setAllowHapticsError;
  }

  if (self.setAllowHapticsError != nil) {
    return NO;
  }

  self.allowHapticsAndSystemSoundsDuringRecording = allowHapticsAndSystemSoundsDuringRecording;
  return YES;
}

- (BOOL)setActive:(BOOL)active withOptions:(AVAudioSessionSetActiveOptions)options error:(NSError **)error
{
  self.setActiveCallCount += 1;
  self.lastSetActiveValue = active;
  self.lastSetActiveOptions = options;

  if (error != nil) {
    *error = self.setActiveError;
  }

  return self.setActiveError == nil;
}

- (BOOL)setPreferredInput:(FakeAudioPortDescription *)inPort error:(NSError **)error
{
  self.setPreferredInputCallCount += 1;
  self.lastPreferredInput = inPort;

  if (error != nil) {
    *error = self.setPreferredInputError;
  }

  return self.setPreferredInputError == nil;
}

@end

@interface TestableAudioSessionManager : AudioSessionManager

@property(nonatomic, assign) BOOL shouldOverrideMicrophoneUsageDescription;
@property(nonatomic, strong) id overriddenMicrophoneUsageDescription;
@property(nonatomic, assign) BOOL shouldUseAudioApplicationRecordPermissionAPI;
@property(nonatomic, assign) BOOL hasOverriddenRecordPermissionStatus;
@property(nonatomic, assign) NSInteger overriddenRecordPermissionStatus;
@property(nonatomic, assign) BOOL overriddenRequestPermissionResult;
@property(nonatomic, assign) NSInteger requestSystemRecordPermissionCallCount;

@end

@implementation TestableAudioSessionManager

- (id)microphoneUsageDescriptionValue
{
  if (self.shouldOverrideMicrophoneUsageDescription) {
    return self.overriddenMicrophoneUsageDescription;
  }

  return [super microphoneUsageDescriptionValue];
}

- (bool)usesAudioApplicationRecordPermissionAPI
{
  return self.shouldUseAudioApplicationRecordPermissionAPI;
}

- (void)requestSystemRecordPermission:(void (^)(BOOL granted))completion
{
  self.requestSystemRecordPermissionCallCount += 1;
  completion(self.overriddenRequestPermissionResult);
}

- (NSInteger)currentRecordPermissionStatus
{
  if (self.hasOverriddenRecordPermissionStatus) {
    return self.overriddenRecordPermissionStatus;
  }

  return [super currentRecordPermissionStatus];
}

@end

@interface AudioSessionManagerTests : XCTestCase

@property(nonatomic, strong) TestableAudioSessionManager *manager;
@property(nonatomic, strong) FakeAVAudioSession *fakeSession;

@end

@implementation AudioSessionManagerTests

- (void)setUp
{
  [super setUp];

  self.manager = [[TestableAudioSessionManager alloc] init];
  self.fakeSession = [[FakeAVAudioSession alloc] init];
  self.manager.audioSession = (AVAudioSession *)self.fakeSession;
  self.manager.shouldOverrideMicrophoneUsageDescription = YES;
  self.manager.overriddenMicrophoneUsageDescription = @"Microphone usage";
}

- (void)tearDown
{
  [self.manager cleanup];
  self.manager = nil;
  self.fakeSession = nil;

  [super tearDown];
}

- (FakeAudioPortDescription *)portWithName:(NSString *)name type:(NSString *)type uid:(NSString *)uid
{
  FakeAudioPortDescription *port = [[FakeAudioPortDescription alloc] init];
  port.portName = name;
  port.portType = type;
  port.UID = uid;
  return port;
}

- (void)testConfigureAudioSessionNoOpsWhenSessionManagementDisabled
{
  self.manager.shouldManageSession = false;
  self.fakeSession.category = AVAudioSessionCategoryRecord;

  XCTAssertTrue([self.manager configureAudioSession]);
  XCTAssertEqual(self.fakeSession.setCategoryCallCount, 0);
  XCTAssertEqual(self.fakeSession.setAllowHapticsCallCount, 0);
}

- (void)testConfigureAudioSessionNoOpsWhenDesiredOptionsAlreadySet
{
  XCTAssertTrue([self.manager areDesiredOptionsSet]);
  XCTAssertTrue([self.manager configureAudioSession]);
  XCTAssertEqual(self.fakeSession.setCategoryCallCount, 0);
  XCTAssertEqual(self.fakeSession.setAllowHapticsCallCount, 0);
}

- (void)testConfigureAudioSessionWritesCategoryModeOptionsAndHaptics
{
  self.manager.desiredCategory = AVAudioSessionCategoryPlayAndRecord;
  self.manager.desiredMode = AVAudioSessionModeVoiceChat;
  self.manager.desiredOptions =
      AVAudioSessionCategoryOptionDuckOthers | AVAudioSessionCategoryOptionDefaultToSpeaker;
  self.manager.allowHapticsAndSounds = true;
  self.fakeSession.category = AVAudioSessionCategoryAmbient;
  self.fakeSession.mode = AVAudioSessionModeMoviePlayback;
  self.fakeSession.categoryOptions = AVAudioSessionCategoryOptionMixWithOthers;
  self.fakeSession.allowHapticsAndSystemSoundsDuringRecording = NO;

  XCTAssertTrue([self.manager configureAudioSession]);
  XCTAssertEqual(self.fakeSession.setCategoryCallCount, 1);
  XCTAssertEqual(self.fakeSession.setAllowHapticsCallCount, 1);
  XCTAssertEqualObjects(self.fakeSession.category, AVAudioSessionCategoryPlayAndRecord);
  XCTAssertEqualObjects(self.fakeSession.mode, AVAudioSessionModeVoiceChat);
  XCTAssertEqual(
      self.fakeSession.categoryOptions,
      AVAudioSessionCategoryOptionDuckOthers | AVAudioSessionCategoryOptionDefaultToSpeaker);
  XCTAssertTrue(self.fakeSession.allowHapticsAndSystemSoundsDuringRecording);
}

- (void)testConfigureAudioSessionReturnsFalseWhenSettingCategoryFails
{
  self.manager.desiredCategory = AVAudioSessionCategoryRecord;
  self.fakeSession.setCategoryError =
      [NSError errorWithDomain:@"AudioSessionTests" code:100 userInfo:nil];

  XCTAssertFalse([self.manager configureAudioSession]);
  XCTAssertEqual(self.fakeSession.setCategoryCallCount, 1);
  XCTAssertEqual(self.fakeSession.setAllowHapticsCallCount, 0);
}

- (void)testConfigureAudioSessionReturnsFalseWhenSettingHapticsFails
{
  self.manager.desiredCategory = AVAudioSessionCategoryPlayAndRecord;
  self.manager.allowHapticsAndSounds = true;
  self.fakeSession.category = AVAudioSessionCategoryAmbient;
  self.fakeSession.setAllowHapticsError =
      [NSError errorWithDomain:@"AudioSessionTests" code:101 userInfo:nil];

  XCTAssertFalse([self.manager configureAudioSession]);
  XCTAssertEqual(self.fakeSession.setCategoryCallCount, 1);
  XCTAssertEqual(self.fakeSession.setAllowHapticsCallCount, 1);
}

- (void)testSetAudioSessionOptionsUpdatesDesiredValuesAndReconfiguresWhenActive
{
  self.manager.isActive = true;
  self.fakeSession.category = AVAudioSessionCategoryAmbient;

  [self.manager setAudioSessionOptions:@"playAndRecord"
                                  mode:@"voiceChat"
                               options:@[ @"duckOthers", @"defaultToSpeaker", @"allowBluetoothHFP" ]
                          allowHaptics:YES
            notifyOthersOnDeactivation:NO];

  XCTAssertEqualObjects(self.manager.desiredCategory, AVAudioSessionCategoryPlayAndRecord);
  XCTAssertEqualObjects(self.manager.desiredMode, AVAudioSessionModeVoiceChat);
  XCTAssertEqual(
      self.manager.desiredOptions,
      AVAudioSessionCategoryOptionDuckOthers | AVAudioSessionCategoryOptionDefaultToSpeaker | 0x4);
  XCTAssertTrue(self.manager.allowHapticsAndSounds);
  XCTAssertFalse(self.manager.notifyOthersOnDeactivation);
  XCTAssertEqual(self.fakeSession.setCategoryCallCount, 1);
}

- (void)testSetAudioSessionOptionsSkipsReconfigureWhenConfigIsUnchanged
{
  self.manager.isActive = true;

  [self.manager setAudioSessionOptions:@"playback"
                                  mode:@"default"
                               options:@[]
                          allowHaptics:NO
            notifyOthersOnDeactivation:NO];

  XCTAssertFalse(self.manager.notifyOthersOnDeactivation);
  XCTAssertEqual(self.fakeSession.setCategoryCallCount, 0);
  XCTAssertEqual(self.fakeSession.setAllowHapticsCallCount, 0);
}

- (void)testSetActiveTrueActivatesSession
{
  NSError *error = nil;

  XCTAssertTrue([self.manager setActive:true error:&error]);
  XCTAssertNil(error);
  XCTAssertTrue(self.manager.isActive);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 1);
  XCTAssertTrue(self.fakeSession.lastSetActiveValue);
  XCTAssertEqual(self.fakeSession.lastSetActiveOptions, (AVAudioSessionSetActiveOptions)0);
}

- (void)testSetActiveFalseNoOpsWhenSessionManagementIsDisabled
{
  self.manager.shouldManageSession = false;
  self.manager.isActive = true;
  NSError *error = nil;

  XCTAssertTrue([self.manager setActive:false error:&error]);
  XCTAssertNil(error);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 0);
  XCTAssertTrue(self.manager.isActive);
}

- (void)testSetActiveFalseNoOpsWhenSessionIsAlreadyInactive
{
  NSError *error = nil;

  XCTAssertTrue([self.manager setActive:false error:&error]);
  XCTAssertNil(error);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 0);
  XCTAssertFalse(self.manager.isActive);
}

- (void)testSetActiveFalseUsesNotifyOthersOnDeactivationWhenEnabled
{
  self.manager.isActive = true;
  self.manager.notifyOthersOnDeactivation = true;

  XCTAssertTrue([self.manager setActive:false error:nil]);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 1);
  XCTAssertFalse(self.fakeSession.lastSetActiveValue);
  XCTAssertEqual(
      self.fakeSession.lastSetActiveOptions,
      AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation);
  XCTAssertFalse(self.manager.isActive);
}

- (void)testSetActiveFalseUsesZeroOptionsWhenNotifyOthersOnDeactivationIsDisabled
{
  self.manager.isActive = true;
  self.manager.notifyOthersOnDeactivation = false;

  XCTAssertTrue([self.manager setActive:false error:nil]);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 1);
  XCTAssertEqual(self.fakeSession.lastSetActiveOptions, (AVAudioSessionSetActiveOptions)0);
}

- (void)testActivateSessionIfNeededRespectsCachedActiveStateAndForce
{
  self.manager.isActive = true;

  XCTAssertTrue([self.manager activateSessionIfNeeded:false error:nil]);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 0);

  XCTAssertTrue([self.manager activateSessionIfNeeded:true error:nil]);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 1);
  XCTAssertTrue(self.manager.isActive);
}

- (void)testActivateSessionIfNeededReturnsFalseWhenConfigurationFails
{
  self.manager.desiredCategory = AVAudioSessionCategoryRecord;
  self.fakeSession.setCategoryError =
      [NSError errorWithDomain:@"AudioSessionTests" code:102 userInfo:nil];

  XCTAssertFalse([self.manager activateSessionIfNeeded:false error:nil]);
  XCTAssertEqual(self.fakeSession.setCategoryCallCount, 1);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 0);
  XCTAssertFalse(self.manager.isActive);
}

- (void)testEnsureActiveNoOpsWhenSessionManagementIsDisabled
{
  self.manager.shouldManageSession = false;

  XCTAssertTrue([self.manager ensureActive:false error:nil]);
  XCTAssertEqual(self.fakeSession.setActiveCallCount, 0);
}

- (void)testInputDiagnosticsSnapshotReportsCurrentRouteAndOwnershipState
{
  self.fakeSession.sampleRate = 48000;
  self.fakeSession.inputNumberOfChannels = 4;
  self.fakeSession.currentRoute.inputs = @[ [self portWithName:@"Mic" type:@"mic" uid:@"mic-1"] ];
  self.fakeSession.currentRoute.outputs =
      @[ [self portWithName:@"Speaker" type:@"speaker" uid:@"spk-1"] ];

  NSString *snapshot = [self.manager inputDiagnosticsSnapshot];

  XCTAssertTrue([snapshot containsString:@"active=false"]);
  XCTAssertTrue([snapshot containsString:@"shouldManage=true"]);
  XCTAssertTrue([snapshot containsString:@"sampleRate=48000.000000"]);
  XCTAssertTrue([snapshot containsString:@"inputChannels=4"]);
  XCTAssertTrue([snapshot containsString:@"Mic(mic,mic-1)"]);
  XCTAssertTrue([snapshot containsString:@"Speaker(speaker,spk-1)"]);
}

- (void)testParseDeviceListMapsPortDescriptionsToDictionaries
{
  NSArray<NSDictionary *> *devices = [self.manager parseDeviceList:@[
    (AVAudioSessionPortDescription *)[self portWithName:@"Mic" type:@"mic" uid:@"mic-1"],
    (AVAudioSessionPortDescription *)[self portWithName:@"Speaker" type:@"speaker" uid:@"spk-1"],
  ]];

  XCTAssertEqualObjects(devices, (@[
    @{
      @"name" : @"Mic",
      @"category" : @"mic",
      @"id" : @"mic-1",
    },
    @{
      @"name" : @"Speaker",
      @"category" : @"speaker",
      @"id" : @"spk-1",
    },
  ]));
}

- (void)testGetDevicesInfoReturnsAvailableAndCurrentRouteSnapshots
{
  FakeAudioPortDescription *availableInput = [self portWithName:@"Available Mic" type:@"mic" uid:@"in-1"];
  FakeAudioPortDescription *currentInput = [self portWithName:@"Current Mic" type:@"mic" uid:@"in-2"];
  FakeAudioPortDescription *currentOutput =
      [self portWithName:@"Speaker" type:@"speaker" uid:@"out-1"];
  self.fakeSession.availableInputs = @[ availableInput ];
  self.fakeSession.currentRoute.inputs = @[ currentInput ];
  self.fakeSession.currentRoute.outputs = @[ currentOutput ];

  __block NSDictionary *resolvedValue = nil;

  [self.manager getDevicesInfo:^(id result) {
    resolvedValue = result;
  }
                     reject:^(NSString *code, NSString *message, NSError *error){
                     }];

  XCTAssertNotNil(resolvedValue);
  XCTAssertEqualObjects(
      resolvedValue[@"availableInputs"],
      (@[
        @{
          @"name" : @"Available Mic",
          @"category" : @"mic",
          @"id" : @"in-1",
        },
      ]));
  XCTAssertEqualObjects(
      resolvedValue[@"currentInputs"],
      (@[
        @{
          @"name" : @"Current Mic",
          @"category" : @"mic",
          @"id" : @"in-2",
        },
      ]));
  XCTAssertEqualObjects(
      resolvedValue[@"availableOutputs"],
      (@[
        @{
          @"name" : @"Speaker",
          @"category" : @"speaker",
          @"id" : @"out-1",
        },
      ]));
  XCTAssertEqualObjects(
      resolvedValue[@"currentOutputs"],
      (@[
        @{
          @"name" : @"Speaker",
          @"category" : @"speaker",
          @"id" : @"out-1",
        },
      ]));
}

- (void)testSetInputDeviceRejectsWhenDeviceIsMissing
{
  self.fakeSession.availableInputs = @[ [self portWithName:@"Mic" type:@"mic" uid:@"mic-1"] ];
  __block NSString *rejectedMessage = nil;

  [self.manager setInputDevice:@"missing-device"
                       resolve:^(id result){
                       }
                        reject:^(NSString *code, NSString *message, NSError *error) {
                          rejectedMessage = message;
                        }];

  XCTAssertEqualObjects(rejectedMessage, @"Input device with id missing-device not found");
  XCTAssertEqual(self.fakeSession.setPreferredInputCallCount, 0);
}

- (void)testSetInputDeviceRejectsWhenSettingPreferredInputFails
{
  FakeAudioPortDescription *input = [self portWithName:@"Mic" type:@"mic" uid:@"mic-1"];
  self.fakeSession.availableInputs = @[ input ];
  self.fakeSession.setPreferredInputError =
      [NSError errorWithDomain:@"AudioSessionTests" code:103 userInfo:nil];
  __block NSString *rejectedMessage = nil;
  __block NSError *rejectedError = nil;

  [self.manager setInputDevice:@"mic-1"
                       resolve:^(id result){
                       }
                        reject:^(NSString *code, NSString *message, NSError *error) {
                          rejectedMessage = message;
                          rejectedError = error;
                        }];

  XCTAssertNotNil(rejectedMessage);
  XCTAssertTrue([rejectedMessage containsString:@"Error while setting preferred input"]);
  XCTAssertEqual(rejectedError.code, 103);
  XCTAssertEqual(self.fakeSession.setPreferredInputCallCount, 1);
}

- (void)testSetInputDeviceResolvesWhenSettingPreferredInputSucceeds
{
  FakeAudioPortDescription *input = [self portWithName:@"Mic" type:@"mic" uid:@"mic-1"];
  self.fakeSession.availableInputs = @[ input ];
  __block id resolvedValue = nil;

  [self.manager setInputDevice:@"mic-1"
                       resolve:^(id result) {
                         resolvedValue = result;
                       }
                        reject:^(NSString *code, NSString *message, NSError *error){
                        }];

  XCTAssertEqualObjects(resolvedValue, @YES);
  XCTAssertEqual(self.fakeSession.setPreferredInputCallCount, 1);
  XCTAssertEqualObjects(self.fakeSession.lastPreferredInput, input);
}

- (void)testRequestRecordingPermissionsReturnsDeniedWhenMicrophoneUsageDescriptionIsMissing
{
  self.manager.overriddenMicrophoneUsageDescription = nil;

  XCTAssertEqualObjects([self.manager requestRecordingPermissions], @"Denied");
  XCTAssertEqual(self.manager.requestSystemRecordPermissionCallCount, 0);
}

- (void)testRequestRecordingPermissionsRejectsWhenMicrophoneUsageDescriptionIsMissing
{
  self.manager.overriddenMicrophoneUsageDescription = nil;
  __block NSString *rejectedMessage = nil;

  [self.manager requestRecordingPermissions:^(id result) {
    XCTFail(@"Expected missing microphone usage description to reject");
  }
                                reject:^(NSString *code, NSString *message, NSError *error) {
                                  rejectedMessage = message;
                                }];

  XCTAssertEqualObjects(
      rejectedMessage,
      @"There is no NSMicrophoneUsageDescription entry in info.plist file. App cannot access microphone without it.");
}

- (void)testRequestRecordingPermissionsUsesOverridableSystemPermissionFlow
{
  self.manager.overriddenRequestPermissionResult = YES;

  XCTAssertEqualObjects([self.manager requestRecordingPermissions], @"Granted");
  XCTAssertEqual(self.manager.requestSystemRecordPermissionCallCount, 1);
}

- (void)testCheckRecordingPermissionsMapsStatusesToUserVisibleValues
{
  self.manager.shouldUseAudioApplicationRecordPermissionAPI = NO;
  self.manager.hasOverriddenRecordPermissionStatus = YES;

  self.manager.overriddenRecordPermissionStatus = AVAudioSessionRecordPermissionUndetermined;
  XCTAssertEqualObjects([self.manager checkRecordingPermissions], @"Undetermined");

  self.manager.overriddenRecordPermissionStatus = AVAudioSessionRecordPermissionGranted;
  XCTAssertEqualObjects([self.manager checkRecordingPermissions], @"Granted");

  self.manager.overriddenRecordPermissionStatus = AVAudioSessionRecordPermissionDenied;
  XCTAssertEqualObjects([self.manager checkRecordingPermissions], @"Denied");
}

@end

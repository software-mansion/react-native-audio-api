#import <AVFAudio/AVFAudio.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation AudioSessionManager

static AudioSessionManager *_sharedInstance = nil;

- (instancetype)init
{
  if (self = [super init]) {
    self.audioSession = [AVAudioSession sharedInstance];

    self.isActive = false;
    self.shouldManageSession = true;

    self.desiredCategory = AVAudioSessionCategoryPlayback;
    self.desiredMode = AVAudioSessionModeDefault;
    self.desiredOptions = 0;
    self.allowHapticsAndSounds = false;
  }

  _sharedInstance = self;
  return self;
}

+ (instancetype)sharedInstance
{
  return _sharedInstance;
}

- (void)cleanup
{
  self.audioSession = nil;
}

- (bool)areDesiredOptionsSet
{
  return (
      self.audioSession.category == self.desiredCategory &&
      self.audioSession.mode == self.desiredMode &&
      self.audioSession.categoryOptions == self.desiredOptions &&
      self.audioSession.allowHapticsAndSystemSoundsDuringRecording == self.allowHapticsAndSounds);
}

- (bool)configureAudioSession
{
  NSError *error = nil;

  if (!self.shouldManageSession || [self areDesiredOptionsSet]) {
    return true;
  }

  [self.audioSession setCategory:self.desiredCategory
                            mode:self.desiredMode
                         options:self.desiredOptions
                           error:&error];

  if (error != nil) {
    NSLog(@"Error while configuring audio session: %@", [error debugDescription]);
    return false;
  } else {
    NSLog(
        @"[AudioSessionManager] Configured audio session: category=%@, mode=%@, options=%lu",
        self.audioSession.category,
        self.audioSession.mode,
        (unsigned long)self.audioSession.categoryOptions);
  }

  if (@available(iOS 13.0, *)) {
    if (self.audioSession.allowHapticsAndSystemSoundsDuringRecording !=
        self.allowHapticsAndSounds) {
      [self.audioSession setAllowHapticsAndSystemSoundsDuringRecording:self.allowHapticsAndSounds
                                                                 error:&error];

      if (error != nil) {
        NSLog(
            @"Error while setting allowHapticsAndSystemSoundsDuringRecording: %@",
            [error debugDescription]);
        return false;
      }
    }
  }

  return true;
}

- (void)setAudioSessionOptions:(NSString *)categoryStr
                          mode:(NSString *)modeStr
                       options:(NSArray *)optionsArray
                  allowHaptics:(BOOL)allowHaptics
{
  AVAudioSessionCategory category = [self categoryFromString:categoryStr];
  AVAudioSessionMode mode = [self modeFromString:modeStr];
  AVAudioSessionCategoryOptions options = [self optionsFromArray:optionsArray];
  bool configChanged = false;

  if (category != self.desiredCategory || mode != self.desiredMode ||
      options != self.desiredOptions || allowHaptics != self.allowHapticsAndSounds) {
    configChanged = true;
  }

  self.desiredCategory = category;
  self.desiredMode = mode;
  self.desiredOptions = options;
  self.allowHapticsAndSounds = allowHaptics;

  if (configChanged && self.isActive) {
    [self configureAudioSession];
  }
}

- (bool)setActive:(bool)active error:(NSError **)error
{
  bool success = false;

  if (!self.shouldManageSession) {
    return true;
  }

  if (self.isActive == active) {
    return true;
  }

  if (active) {
    success = [self configureAudioSession];

    if (!success) {
      return false;
    }
  }

  success = [self.audioSession setActive:active error:error];

  if (success) {
    self.isActive = active;
  }

  return success;
}

- (void)markInactive
{
  // Mark as inactive no matter the state reported by AVAudioSession,
  // this is used during interruptions to "force" going through configure&activate flow
  // which is necessary after some of the interruptions (f.e. when the other app re-configures the hardware)
  self.isActive = false;
}

- (void)disableSessionManagement
{
  self.shouldManageSession = false;
}

- (NSNumber *)getDevicePreferredSampleRate
{
  return [NSNumber numberWithFloat:[self.audioSession sampleRate]];
}

- (NSNumber *)getDevicePreferredInputChannelCount
{
  return [NSNumber numberWithInteger:[self.audioSession inputNumberOfChannels]];
}

- (void)requestRecordingPermissions:(RCTPromiseResolveBlock)resolve
                             reject:(RCTPromiseRejectBlock)reject
{
  id value = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSMicrophoneUsageDescription"];
  // if there is no entry NSMicrophoneUsageDescription calling
  // requestRecordPermission will quit an app
  if (value == nil) {
    reject(
        nil,
        @"There is no NSMicrophoneUsageDescription entry in info.plist file. App cannot access microphone without it.",
        nil);
    return;
  }

  resolve([self requestRecordingPermissions]);
}

- (NSString *)requestRecordingPermissions
{
  id value = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSMicrophoneUsageDescription"];

  if (value == nil) {
    return @"Denied";
  }

  __block NSString *result = @"Denied";
  dispatch_semaphore_t sem = dispatch_semaphore_create(0);

#if TARGET_OS_SIMULATOR
  [self.audioSession requestRecordPermission:^(BOOL granted) {
    result = granted ? @"Granted" : @"Denied";
    dispatch_semaphore_signal(sem);
  }];

  dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
  return result;
#else
  if (@available(iOS 17.0, *)) {
    [AVAudioApplication requestRecordPermissionWithCompletionHandler:^(BOOL granted) {
      result = granted ? @"Granted" : @"Denied";
      dispatch_semaphore_signal(sem);
    }];
  } else {
    [self.audioSession requestRecordPermission:^(BOOL granted) {
      result = granted ? @"Granted" : @"Denied";
      dispatch_semaphore_signal(sem);
    }];
  }

  dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
  return result;
#endif
}

- (void)checkRecordingPermissions:(RCTPromiseResolveBlock)resolve
                           reject:(RCTPromiseRejectBlock)reject
{
  resolve([self checkRecordingPermissions]);
}

- (NSString *)checkRecordingPermissions
{
#if TARGET_OS_SIMULATOR
  NSInteger res = [self.audioSession recordPermission];

  switch (res) {
    case AVAudioSessionRecordPermissionUndetermined:
      return @"Undetermined";
    case AVAudioSessionRecordPermissionGranted:
      return @"Granted";
    case AVAudioSessionRecordPermissionDenied:
      return @"Denied";
    default:
      return @"Undetermined";
  }
#else
  if (@available(iOS 17, *)) {
    NSInteger res = [[AVAudioApplication sharedInstance] recordPermission];

    switch (res) {
      case AVAudioApplicationRecordPermissionUndetermined:
        return @"Undetermined";
      case AVAudioApplicationRecordPermissionGranted:
        return @"Granted";
      case AVAudioApplicationRecordPermissionDenied:
        return @"Denied";
      default:
        return @"Undetermined";
    }
  }

  NSInteger res = [self.audioSession recordPermission];

  switch (res) {
    case AVAudioSessionRecordPermissionUndetermined:
      return @"Undetermined";
    case AVAudioSessionRecordPermissionGranted:
      return @"Granted";
    case AVAudioSessionRecordPermissionDenied:
      return @"Denied";
    default:
      return @"Undetermined";
  }
#endif
}

- (void)getDevicesInfo:(RCTPromiseResolveBlock)resolve reject:(RCTPromiseRejectBlock)reject
{
  NSMutableDictionary *devicesInfo = [[NSMutableDictionary alloc] init];

  [devicesInfo setValue:[self parseDeviceList:[self.audioSession availableInputs]]
                 forKey:@"availableInputs"];
  [devicesInfo setValue:[self parseDeviceList:[[self.audioSession currentRoute] inputs]]
                 forKey:@"currentInputs"];
  [devicesInfo setValue:[self parseDeviceList:[[self.audioSession currentRoute] outputs]]
                 forKey:@"availableOutputs"];
  [devicesInfo setValue:[self parseDeviceList:[[self.audioSession currentRoute] outputs]]
                 forKey:@"currentOutputs"];

  resolve(devicesInfo);
}

- (NSArray<NSDictionary *> *)parseDeviceList:(NSArray<AVAudioSessionPortDescription *> *)devices
{
  NSMutableArray<NSDictionary *> *deviceList = [[NSMutableArray alloc] init];

  for (AVAudioSessionPortDescription *device in devices) {
    [deviceList addObject:@{
      @"name" : device.portName,
      @"category" : device.portType,
      @"id" : device.UID,
    }];
  }

  return deviceList;
}

- (void)setInputDevice:(NSString *)deviceId
               resolve:(RCTPromiseResolveBlock)resolve
                reject:(RCTPromiseRejectBlock)reject
{
  NSError *error = nil;
  NSArray<AVAudioSessionPortDescription *> *availableInputs = [self.audioSession availableInputs];

  AVAudioSessionPortDescription *selectedInput = nil;

  for (AVAudioSessionPortDescription *input in availableInputs) {
    if ([input.UID isEqualToString:deviceId]) {
      selectedInput = input;
      break;
    }
  }

  if (selectedInput == nil) {
    reject(nil, [NSString stringWithFormat:@"Input device with id %@ not found", deviceId], nil);
    return;
  }

  [self.audioSession setPreferredInput:selectedInput error:&error];

  if (error != nil) {
    reject(
        nil,
        [NSString
            stringWithFormat:@"Error while setting preferred input: %@", [error debugDescription]],
        error);
    return;
  }

  resolve(@(true));
}

- (AVAudioSessionCategory)categoryFromString:(NSString *)categorySTR
{
  AVAudioSessionCategory category = 0;

  if ([categorySTR isEqualToString:@"record"]) {
    category = AVAudioSessionCategoryRecord;
  } else if ([categorySTR isEqualToString:@"ambient"]) {
    category = AVAudioSessionCategoryAmbient;
  } else if ([categorySTR isEqualToString:@"playback"]) {
    category = AVAudioSessionCategoryPlayback;
  } else if ([categorySTR isEqualToString:@"multiRoute"]) {
    category = AVAudioSessionCategoryMultiRoute;
  } else if ([categorySTR isEqualToString:@"soloAmbient"]) {
    category = AVAudioSessionCategorySoloAmbient;
  } else if ([categorySTR isEqualToString:@"playAndRecord"]) {
    category = AVAudioSessionCategoryPlayAndRecord;
  }

  return category;
}

- (AVAudioSessionMode)modeFromString:(NSString *)modeSTR
{
  AVAudioSessionMode mode = 0;

  if ([modeSTR isEqualToString:@"default"]) {
    mode = AVAudioSessionModeDefault;
  } else if ([modeSTR isEqualToString:@"gameChat"]) {
    mode = AVAudioSessionModeGameChat;
  } else if ([modeSTR isEqualToString:@"videoChat"]) {
    mode = AVAudioSessionModeVideoChat;
  } else if ([modeSTR isEqualToString:@"voiceChat"]) {
    mode = AVAudioSessionModeVoiceChat;
  } else if ([modeSTR isEqualToString:@"measurement"]) {
    mode = AVAudioSessionModeMeasurement;
  } else if ([modeSTR isEqualToString:@"voicePrompt"]) {
    mode = AVAudioSessionModeVoicePrompt;
  } else if ([modeSTR isEqualToString:@"spokenAudio"]) {
    mode = AVAudioSessionModeSpokenAudio;
  } else if ([modeSTR isEqualToString:@"moviePlayback"]) {
    mode = AVAudioSessionModeMoviePlayback;
  } else if ([modeSTR isEqualToString:@"videoRecording"]) {
    mode = AVAudioSessionModeVideoRecording;
  }

  return mode;
}

- (AVAudioSessionCategoryOptions)optionsFromArray:(NSArray *)optionsArray
{
  AVAudioSessionCategoryOptions options = 0;

  for (NSString *option in optionsArray) {
    if ([option isEqualToString:@"duckOthers"]) {
      options |= AVAudioSessionCategoryOptionDuckOthers;
    }

    if ([option isEqualToString:@"allowAirPlay"]) {
      options |= AVAudioSessionCategoryOptionAllowAirPlay;
    }

    if ([option isEqualToString:@"mixWithOthers"]) {
      options |= AVAudioSessionCategoryOptionMixWithOthers;
    }

    if ([option isEqualToString:@"allowBluetoothHFP"]) {
      options |= AVAudioSessionCategoryOptionAllowBluetoothHFP;
    }

    if ([option isEqualToString:@"defaultToSpeaker"]) {
      options |= AVAudioSessionCategoryOptionDefaultToSpeaker;
    }

    if ([option isEqualToString:@"allowBluetoothA2DP"]) {
      options |= AVAudioSessionCategoryOptionAllowBluetoothA2DP;
    }

    if ([option isEqualToString:@"overrideMutedMicrophoneInterruption"]) {
      options |= AVAudioSessionCategoryOptionOverrideMutedMicrophoneInterruption;
    }

    if ([option isEqualToString:@"interruptSpokenAudioAndMixWithOthers"]) {
      options |= AVAudioSessionCategoryOptionInterruptSpokenAudioAndMixWithOthers;
    }
  }

  return options;
}

- (bool)isSessionActive
{
  return self.isActive;
}

@end

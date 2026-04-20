#import <AVFAudio/AVFAudio.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@interface AudioSessionManager ()

- (id)microphoneUsageDescriptionValue;
- (bool)usesAudioApplicationRecordPermissionAPI;
- (void)requestSystemRecordPermission:(void (^)(BOOL granted))completion;
- (NSInteger)currentRecordPermissionStatus;
- (NSString *)recordPermissionStatusString:(NSInteger)status;
- (NSString *)formatPorts:(NSArray<AVAudioSessionPortDescription *> *)ports;

@end

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
    self.notifyOthersOnDeactivation = true;
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
    notifyOthersOnDeactivation:(BOOL)notifyOthersOnDeactivation
{
  AVAudioSessionCategory category = [self categoryFromString:categoryStr];
  AVAudioSessionMode mode = [self modeFromString:modeStr];
  AVAudioSessionCategoryOptions options = [self optionsFromArray:optionsArray];
  bool configChanged = false;

  if (category != self.desiredCategory || mode != self.desiredMode ||
      options != self.desiredOptions || allowHaptics != self.allowHapticsAndSounds) {
    configChanged = true;
  }

  if (configChanged) {
    AudioEngine *audioEngine = [AudioEngine sharedInstance];
    audioEngine.sessionDeactivationInvalidatedGraph = YES;
  }

  self.desiredCategory = category;
  self.desiredMode = mode;
  self.desiredOptions = options;
  self.allowHapticsAndSounds = allowHaptics;
  self.notifyOthersOnDeactivation = notifyOthersOnDeactivation;

  if (configChanged && self.isActive) {
    [self configureAudioSession];
  }
}

- (bool)setActive:(bool)active error:(NSError **)error
{
  if (active) {
    return [self ensureActive:false error:error];
  }

  if (!self.shouldManageSession) {
    return true;
  }

  if (!self.isActive) {
    return true;
  }

  AVAudioSessionSetActiveOptions options = active
      ? 0
      : (self.notifyOthersOnDeactivation ? AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                         : 0);
  bool success = [self.audioSession setActive:active withOptions:options error:error];

  if (success) {
    self.isActive = active;
  }

  return success;
}

- (bool)ensureActive:(bool)force error:(NSError **)error
{
  return [self activateSessionIfNeeded:force error:error];
}

- (bool)activateSessionIfNeeded:(bool)force error:(NSError **)error
{
  if (!self.shouldManageSession) {
    return true;
  }

  if (self.isActive && !force) {
    return true;
  }

  if (![self configureAudioSession]) {
    return false;
  }

  bool success = [self.audioSession setActive:true withOptions:0 error:error];

  if (success) {
    self.isActive = true;
  }

  return success;
}

- (void)markInactive
{
  // AVAudioSession does not expose a reliable active-state query, so drop our cached flag and
  // force the next audio operation to re-assert activation.
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

- (NSString *)inputDiagnosticsSnapshot
{
  AVAudioSessionRouteDescription *route = [self.audioSession currentRoute];
  NSArray<AVAudioSessionPortDescription *> *inputs = route != nil ? route.inputs : @[];
  NSArray<AVAudioSessionPortDescription *> *outputs = route != nil ? route.outputs : @[];

  return [NSString stringWithFormat:
                       @"session={active=%@, shouldManage=%@, category=%@, mode=%@, options=%lu, "
                       @"sampleRate=%f, inputChannels=%lu}; route={inputs=%@, outputs=%@}",
                       self.isActive ? @"true" : @"false",
                       self.shouldManageSession ? @"true" : @"false",
                       self.audioSession.category ?: @"(null)",
                       self.audioSession.mode ?: @"(null)",
                       (unsigned long)self.audioSession.categoryOptions,
                       self.audioSession.sampleRate,
                       (unsigned long)self.audioSession.inputNumberOfChannels,
                       [self formatPorts:inputs],
                       [self formatPorts:outputs]];
}

- (id)microphoneUsageDescriptionValue
{
  return [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSMicrophoneUsageDescription"];
}

- (bool)usesAudioApplicationRecordPermissionAPI
{
#if TARGET_OS_SIMULATOR
  return false;
#else
  if (@available(iOS 17.0, *)) {
    return true;
  }

  return false;
#endif
}

- (void)requestSystemRecordPermission:(void (^)(BOOL granted))completion
{
  if ([self usesAudioApplicationRecordPermissionAPI]) {
    if (@available(iOS 17.0, *)) {
      [AVAudioApplication requestRecordPermissionWithCompletionHandler:completion];
      return;
    }
  }

  [self.audioSession requestRecordPermission:completion];
}

- (NSInteger)currentRecordPermissionStatus
{
  if ([self usesAudioApplicationRecordPermissionAPI]) {
    if (@available(iOS 17.0, *)) {
      return [[AVAudioApplication sharedInstance] recordPermission];
    }
  }

  return [self.audioSession recordPermission];
}

- (NSString *)recordPermissionStatusString:(NSInteger)status
{
  if ([self usesAudioApplicationRecordPermissionAPI]) {
    switch (status) {
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

  switch (status) {
    case AVAudioSessionRecordPermissionUndetermined:
      return @"Undetermined";
    case AVAudioSessionRecordPermissionGranted:
      return @"Granted";
    case AVAudioSessionRecordPermissionDenied:
      return @"Denied";
    default:
      return @"Undetermined";
  }
}

- (NSString *)formatPorts:(NSArray<AVAudioSessionPortDescription *> *)ports
{
  if (ports.count == 0) {
    return @"[]";
  }

  NSMutableArray<NSString *> *formattedPorts =
      [[NSMutableArray alloc] initWithCapacity:ports.count];

  for (AVAudioSessionPortDescription *port in ports) {
    [formattedPorts addObject:[NSString stringWithFormat:@"%@(%@,%@)",
                                                         port.portName ?: @"unknown",
                                                         port.portType ?: @"unknown",
                                                         port.UID ?: @"unknown"]];
  }

  return [NSString stringWithFormat:@"[%@]", [formattedPorts componentsJoinedByString:@", "]];
}

- (void)requestRecordingPermissions:(RCTPromiseResolveBlock)resolve
                             reject:(RCTPromiseRejectBlock)reject
{
  id value = [self microphoneUsageDescriptionValue];
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
  id value = [self microphoneUsageDescriptionValue];

  if (value == nil) {
    return @"Denied";
  }

  __block NSString *result = @"Denied";
  dispatch_semaphore_t sem = dispatch_semaphore_create(0);

  [self requestSystemRecordPermission:^(BOOL granted) {
    result = granted ? @"Granted" : @"Denied";
    dispatch_semaphore_signal(sem);
  }];

  dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
  return result;
}

- (void)checkRecordingPermissions:(RCTPromiseResolveBlock)resolve
                           reject:(RCTPromiseRejectBlock)reject
{
  resolve([self checkRecordingPermissions]);
}

- (NSString *)checkRecordingPermissions
{
  return [self recordPermissionStatusString:[self currentRecordPermissionStatus]];
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

  if ([categorySTR isEqualToString:@"ambient"]) {
    category = AVAudioSessionCategoryAmbient;
  } else if ([categorySTR isEqualToString:@"multiRoute"]) {
    category = AVAudioSessionCategoryMultiRoute;
  } else if ([categorySTR isEqualToString:@"playAndRecord"]) {
    category = AVAudioSessionCategoryPlayAndRecord;
  } else if ([categorySTR isEqualToString:@"playback"]) {
    category = AVAudioSessionCategoryPlayback;
  } else if ([categorySTR isEqualToString:@"record"]) {
    category = AVAudioSessionCategoryRecord;
  } else if ([categorySTR isEqualToString:@"soloAmbient"]) {
    category = AVAudioSessionCategorySoloAmbient;
  }

  return category;
}

- (AVAudioSessionMode)modeFromString:(NSString *)modeSTR
{
  AVAudioSessionMode mode = 0;

  if ([modeSTR isEqualToString:@"default"]) {
    mode = AVAudioSessionModeDefault;
  } else if ([modeSTR isEqualToString:@"dualRoute"]) {
    if (@available(iOS 26.2, *)) {
      mode = AVAudioSessionModeDualRoute;
    } else {
      mode = AVAudioSessionModeDefault;
    }
  } else if ([modeSTR isEqualToString:@"gameChat"]) {
    mode = AVAudioSessionModeGameChat;
  } else if ([modeSTR isEqualToString:@"measurement"]) {
    mode = AVAudioSessionModeMeasurement;
  } else if ([modeSTR isEqualToString:@"moviePlayback"]) {
    mode = AVAudioSessionModeMoviePlayback;
  } else if ([modeSTR isEqualToString:@"shortFormVideo"]) {
    if (@available(iOS 26, *)) {
      mode = AVAudioSessionModeShortFormVideo;
    } else {
      mode = AVAudioSessionModeDefault;
    }
  } else if ([modeSTR isEqualToString:@"spokenAudio"]) {
    mode = AVAudioSessionModeSpokenAudio;
  } else if ([modeSTR isEqualToString:@"videoChat"]) {
    mode = AVAudioSessionModeVideoChat;
  } else if ([modeSTR isEqualToString:@"videoRecording"]) {
    mode = AVAudioSessionModeVideoRecording;
  } else if ([modeSTR isEqualToString:@"voiceChat"]) {
    mode = AVAudioSessionModeVoiceChat;
  } else if ([modeSTR isEqualToString:@"voicePrompt"]) {
    mode = AVAudioSessionModeVoicePrompt;
  }

  return mode;
}

- (AVAudioSessionCategoryOptions)optionsFromArray:(NSArray *)optionsArray
{
  AVAudioSessionCategoryOptions options = 0;

  for (NSString *option in optionsArray) {
    if ([option isEqualToString:@"allowAirPlay"]) {
      options |= AVAudioSessionCategoryOptionAllowAirPlay;
      continue;
    }

    if ([option isEqualToString:@"allowBluetoothA2DP"]) {
      options |= AVAudioSessionCategoryOptionAllowBluetoothA2DP;
      continue;
    }

    if ([option isEqualToString:@"allowBluetoothHFP"]) {
      options |= AVAudioSessionCategoryOptionAllowBluetoothHFP;
      continue;
    }

    if ([option isEqualToString:@"bluetoothHighQualityRecording"]) {
      if (@available(iOS 26, *)) {
        options |= AVAudioSessionCategoryOptionBluetoothHighQualityRecording;
      }
      continue;
    }

    if ([option isEqualToString:@"defaultToSpeaker"]) {
      options |= AVAudioSessionCategoryOptionDefaultToSpeaker;
      continue;
    }

    if ([option isEqualToString:@"duckOthers"]) {
      options |= AVAudioSessionCategoryOptionDuckOthers;
      continue;
    }

    if ([option isEqualToString:@"farFieldInput"]) {
      if (@available(iOS 26.2, *)) {
        options |= AVAudioSessionCategoryOptionFarFieldInput;
        continue;
      }
    }

    if ([option isEqualToString:@"interruptSpokenAudioAndMixWithOthers"]) {
      options |= AVAudioSessionCategoryOptionInterruptSpokenAudioAndMixWithOthers;
      continue;
    }

    if ([option isEqualToString:@"mixWithOthers"]) {
      options |= AVAudioSessionCategoryOptionMixWithOthers;
      continue;
    }

    if ([option isEqualToString:@"overrideMutedMicrophoneInterruption"]) {
      options |= AVAudioSessionCategoryOptionOverrideMutedMicrophoneInterruption;
      continue;
    }
  }

  return options;
}
@end

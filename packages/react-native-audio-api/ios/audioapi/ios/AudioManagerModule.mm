#import <audioapi/ios/AudioManagerModule.h>

#import <React/RCTBridge+Private.h>

using namespace facebook::react;

@interface RCTBridge (JSIRuntime)
- (void *)runtime;
@end

@implementation AudioManagerModule

RCT_EXPORT_MODULE(AudioManagerModule);

#ifdef RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeAudioManagerModuleSpecJSI>(params);
}
#endif // RCT_NEW_ARCH_ENABLED

- (id) init
{
  if (self == [super init]) {
    self.audioManager = [[AudioManager alloc] init];
  }

  return self;
}

- (void)invalidate {
  [self.audioManager cleanup];
  [super invalidate];
}

- (void)setNowPlaying:(NSDictionary *)info {
  [self.audioManager setNowPlaying:info];
}

- (void)setSessionCategory:(NSString *)category
{
  AVAudioSessionCategory sessionCategory;

  if ([category isEqualToString:@"record"]) {
    sessionCategory = AVAudioSessionCategoryRecord;
    return;
  }

  if ([category isEqualToString:@"ambient"]) {
    sessionCategory = AVAudioSessionCategoryAmbient;
    return;
  }

  if ([category isEqualToString:@"playback"]) {
    sessionCategory = AVAudioSessionCategoryPlayback;
    return;
  }

  if ([category isEqualToString:@"multiRoute"]) {
    sessionCategory = AVAudioSessionCategoryMultiRoute;
    return;
  }

  if ([category isEqualToString:@"soloAmbient"]) {
    sessionCategory = AVAudioSessionCategorySoloAmbient;
    return;
  }

  if ([category isEqualToString:@"playAndRecord"]) {
    sessionCategory = AVAudioSessionCategoryPlayAndRecord;
    return;
  }

  [self.audioManager setSessionCategory:sessionCategory];
}

- (void)setSessionMode:(NSString *)mode {
  AVAudioSessionMode sessionMode;

  if([mode isEqualToString:@"default"]) {
    sessionMode = AVAudioSessionModeDefault;
    return;
  }

  if ([mode isEqualToString:@"gameChat"]) {
    sessionMode = AVAudioSessionModeGameChat;
    return;
  }

  if ([mode isEqualToString:@"videoChat"]) {
    sessionMode = AVAudioSessionModeVideoChat;
    return;
  }

  if([mode isEqualToString:@"voiceChat"]) {
    sessionMode = AVAudioSessionModeVoiceChat;
    return;
  }

  if ([mode isEqualToString:@"measurement"]) {
    sessionMode = AVAudioSessionModeMeasurement;
    return;
  }

  if ([mode isEqualToString:@"voicePrompt"]) {
    sessionMode = AVAudioSessionModeVoicePrompt;
    return;
  }

  if ([mode isEqualToString:@"spokenAudio"]) {
    sessionMode = AVAudioSessionModeSpokenAudio;
    return;
  }

  if ([mode isEqualToString:@"moviePlayback"]) {
    sessionMode = AVAudioSessionModeMoviePlayback;
    return;
  }

  if([mode isEqualToString:@"videoRecording"]) {
    sessionMode = AVAudioSessionModeVideoRecording;
    return;
  }

  [self.audioManager setSessionMode:sessionMode];
}

- (void)setSessionCategoryOptions:(NSArray *)options {
  AVAudioSessionCategoryOptions sessionOptions = 0; // No options set initially

    for (NSString *option in options) {
        if ([option isEqualToString:@"duckOthers"]) {
            sessionOptions |= AVAudioSessionCategoryOptionDuckOthers;
        } else if ([option isEqualToString:@"allowAirPlay"]) {
            sessionOptions |= AVAudioSessionCategoryOptionAllowAirPlay;
        } else if ([option isEqualToString:@"mixWithOthers"]) {
            sessionOptions |= AVAudioSessionCategoryOptionMixWithOthers;
        } else if ([option isEqualToString:@"allowBluetooth"]) {
            sessionOptions |= AVAudioSessionCategoryOptionAllowBluetooth;
        } else if ([option isEqualToString:@"defaultToSpeaker"]) {
            sessionOptions |= AVAudioSessionCategoryOptionDefaultToSpeaker;
        } else if ([option isEqualToString:@"allowBluetoothA2DP"]) {
            sessionOptions |= AVAudioSessionCategoryOptionAllowBluetoothA2DP;
        } else if ([option isEqualToString:@"overrideMutedMicrophoneInterruption"]) {
            sessionOptions |= AVAudioSessionCategoryOptionOverrideMutedMicrophoneInterruption;
        } else if ([option isEqualToString:@"interruptSpokenAudioAndMixWithOthers"]) {
            sessionOptions |= AVAudioSessionCategoryOptionInterruptSpokenAudioAndMixWithOthers;
        }
    }
  
  [self.audioManager setSessionOptions:sessionOptions];
}

- (NSNumber *)getSampleRate
{
  return [NSNumber numberWithFloat:[self.audioManager getSampleRate]];
}

@end


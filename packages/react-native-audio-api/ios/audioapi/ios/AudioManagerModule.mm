#import <audioapi/ios/AudioManagerModule.h>

#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/LockScreenManager.h>
#import <audioapi/ios/system/NotificationManager.h>

@implementation AudioManagerModule

RCT_EXPORT_MODULE(AudioManagerModule);

#ifdef RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeAudioManagerModuleSpecJSI>(params);
}
#endif // RCT_NEW_ARCH_ENABLED

- (id)init
{
  if (self == [super init]) {
    NSLog(@"[AudioManager] init");

    self.audioEngine = [AudioEngine sharedInstance];
    self.notificationManager = [NotificationManager sharedInstance];
    self.audioSessionManager = [AudioSessionManager sharedInstance];
    self.lockScreenManager = [LockScreenManager sharedInstanceWithAudioManagerModule:self];
  }

  return self;
}

- (void)cleanup
{
  NSLog(@"[AudioManager] cleanup");
  [self.audioEngine cleanup];
  [self.notificationManager cleanup];
  [self.audioSessionManager cleanup];
  [self.lockScreenManager cleanup];
}

- (void)dealloc
{
  [self cleanup];
}

- (void)setLockScreenInfo:(NSDictionary *)info
{
  [self.lockScreenManager setLockScreenInfo:info];
}

- (void)resetLockScreenInfo
{
  [self.lockScreenManager resetLockScreenInfo];
}
- (void)setSessionOptions:(NSString *)category mode:(NSString *)mode options:(NSArray *)options active:(BOOL)active
{
  NSError *error = nil;

  if (active) {
    [self.audioSessionManager setSessionOptions:category mode:mode options:options];
  } else {
    [self.audioSessionManager setActive:false error:&error];
  }
}

- (NSNumber *)getDevicePreferredSampleRate
{
  return [self.audioSessionManager getDevicePreferredSampleRate];
}

@end

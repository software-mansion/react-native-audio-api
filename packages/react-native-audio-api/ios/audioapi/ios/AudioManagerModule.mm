#import <audioapi/ios/AudioManagerModule.h>

#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/LockScreenManager.h>
#import <audioapi/ios/system/NotificationManager.h>

@implementation AudioManagerModule

RCT_EXPORT_MODULE(AudioManagerModule);

- (void)invalidate
{
  [self.audioEngine cleanup];
  [self.notificationManager cleanup];
  [self.audioSessionManager cleanup];
  [self.lockScreenManager cleanup];
  
  [super invalidate];
}

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install)
{
  self.audioSessionManager = [[AudioSessionManager alloc] init];
  self.audioEngine = [[AudioEngine alloc] initWithAudioSessionManager:self.audioSessionManager];
  self.lockScreenManager = [[LockScreenManager alloc] initWithAudioManagerModule:self];
  self.notificationManager = [[NotificationManager alloc] initWithAudioManagerModule:self];
  
  return @true;
}

RCT_EXPORT_METHOD(setLockScreenInfo : (NSDictionary *)info)
{
  [self.lockScreenManager setLockScreenInfo:info];
}

RCT_EXPORT_METHOD(resetLockScreenInfo)
{
  [self.lockScreenManager resetLockScreenInfo];
}

RCT_EXPORT_METHOD(enableRemoteCommand : (NSString *)name enabled : (BOOL)enabled)
{
  [self.lockScreenManager enableRemoteCommand:name enabled:enabled];
}

RCT_EXPORT_METHOD(setAudioSessionOptions : (NSString *)category mode : (NSString *)mode options : (NSArray *)options)
{
  [self.audioSessionManager setAudioSessionOptions:category mode:mode options:options];
}

RCT_EXPORT_METHOD(observeAudioInterruptions : (BOOL)enabled)
{
  [self.notificationManager observeAudioInterruptions:enabled];
}

RCT_EXPORT_METHOD(observeVolumeChanges : (BOOL)enabled)
{
  [self.notificationManager observeVolumeChanges:(BOOL)enabled];
}

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(getDevicePreferredSampleRate)
{
  return [self.audioSessionManager getDevicePreferredSampleRate];
}

RCT_EXPORT_METHOD(requestRecordingPermissions : (RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)
                      reject)
{
  NSString *res = [self.audioSessionManager requestRecordingPermissions];
  resolve(res);
}

RCT_EXPORT_METHOD(checkRecordingPermissions : (RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject)
{
  NSString *res = [self.audioSessionManager checkRecordingPermissions];
  resolve(res);
}

- (NSArray<NSString *> *)supportedEvents
{
  return @[
    @"onRemotePlay",
    @"onRemotePause",
    @"onRemoteStop",
    @"onRemoteTogglePlayPause",
    @"onRemoteChangePlaybackRate",
    @"onRemoteNextTrack",
    @"onRemotePreviousTrack",
    @"onRemoteSkipForward",
    @"onRemoteSkipBackward",
    @"onRemoteSeekForward",
    @"onRemoteSeekBackward",
    @"onRemoteChangePlaybackPosition",
    @"onInterruption",
    @"onRouteChange",
    @"onVolumeChange"
  ];
}

@end

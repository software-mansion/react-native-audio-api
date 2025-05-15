#import <React/RCTBridge+Private.h>
#import <audioapi/ios/AudioAPIModule.h>
#ifdef RCT_NEW_ARCH_ENABLED
#import <React/RCTCallInvoker.h>
#endif // RCT_NEW_ARCH_ENABLED

#import <audioapi/AudioAPIModuleInstaller.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/LockScreenManager.h>
#import <audioapi/ios/system/NotificationManager.h>

#import <audioapi/events/AudioEventHandlerRegistry.h>

using namespace audioapi;
using namespace facebook::react;

@interface RCTBridge (JSIRuntime)
- (void *)runtime;
@end

#if defined(RCT_NEW_ARCH_ENABLED)
// nothing
#else // defined(RCT_NEW_ARCH_ENABLED)
@interface RCTBridge (RCTTurboModule)
- (std::shared_ptr<facebook::react::CallInvoker>)jsCallInvoker;
- (void)_tryAndHandleError:(dispatch_block_t)block;
@end
#endif // RCT_NEW_ARCH_ENABLED

@implementation AudioAPIModule {
  std::shared_ptr<AudioEventHandlerRegistry> _eventHandler;
}

#if defined(RCT_NEW_ARCH_ENABLED)
@synthesize callInvoker = _callInvoker;
#endif // defined(RCT_NEW_ARCH_ENABLED)

RCT_EXPORT_MODULE(AudioAPIModule);

- (void)invalidate
{
  [self.audioEngine cleanup];
  [self.notificationManager cleanup];
  [self.audioSessionManager cleanup];
  [self.lockScreenManager cleanup];

  _eventHandler = nullptr;

  [super invalidate];
}

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install)
{
  self.audioSessionManager = [[AudioSessionManager alloc] init];
  self.audioEngine = [[AudioEngine alloc] initWithAudioSessionManager:self.audioSessionManager];
  self.lockScreenManager = [[LockScreenManager alloc] initWithAudioAPIModule:self];
  self.notificationManager = [[NotificationManager alloc] initWithAudioAPIModule:self];

  auto jsiRuntime = reinterpret_cast<facebook::jsi::Runtime *>(self.bridge.runtime);

#if defined(RCT_NEW_ARCH_ENABLED)
  auto jsCallInvoker = _callInvoker.callInvoker;
#else // defined(RCT_NEW_ARCH_ENABLED)
  auto jsCallInvoker = self.bridge.jsCallInvoker;
#endif // defined(RCT_NEW_ARCH_ENABLED)

  assert(jsiRuntime != nullptr);

  _eventHandler = std::make_shared<AudioEventHandlerRegistry>(jsiRuntime, jsCallInvoker);

  self.audioSessionManager = [[AudioSessionManager alloc] init];
  self.audioEngine = [[AudioEngine alloc] initWithAudioSessionManager:self.audioSessionManager];
  self.lockScreenManager = [[LockScreenManager alloc] initWithAudioAPIModule:self];
  self.notificationManager = [[NotificationManager alloc] initWithAudioAPIModule:self];

  audioapi::AudioAPIModuleInstaller::injectJSIBindings(jsiRuntime, jsCallInvoker, _eventHandler);

  NSLog(@"Successfully installed JSI bindings for react-native-audio-api!");
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

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(getDevicePreferredSampleRate)
{
  return [self.audioSessionManager getDevicePreferredSampleRate];
}

RCT_EXPORT_METHOD(observeAudioInterruptions : (BOOL)enabled)
{
  [self.notificationManager observeAudioInterruptions:enabled];
}

RCT_EXPORT_METHOD(observeVolumeChanges : (BOOL)enabled)
{
  [self.notificationManager observeVolumeChanges:(BOOL)enabled];
}

RCT_EXPORT_METHOD(requestRecordingPermissions : (nonnull RCTPromiseResolveBlock)
                      resolve reject : (nonnull RCTPromiseRejectBlock)reject)
{
  NSString *res = [self.audioSessionManager requestRecordingPermissions];
  resolve(res);
}

RCT_EXPORT_METHOD(checkRecordingPermissions : (nonnull RCTPromiseResolveBlock)
                      resolve reject : (nonnull RCTPromiseRejectBlock)reject)
{
  NSString *res = [self.audioSessionManager checkRecordingPermissions];
  resolve(res);
}

#ifdef RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeAudioAPIModuleSpecJSI>(params);
}
#endif // RCT_NEW_ARCH_ENABLED

- (void)invokeHandlerWithEventName:(NSString *)eventName eventBody:(NSDictionary *)eventBody
{
  auto name = [eventName UTF8String];

  std::unordered_map<std::string, EventValue> body = {};

  for (NSString *key in eventBody) {
    id value = eventBody[key];
    std::string stdKey = [key UTF8String];

    if ([value isKindOfClass:[NSString class]]) {
      std::string stdValue = [value UTF8String];
      body[stdKey] = EventValue(stdValue);
    } else if ([value isKindOfClass:[NSNumber class]]) {
      const char *type = [value objCType];
      if (strcmp(type, @encode(int)) == 0) {
        body[stdKey] = EventValue([value intValue]);
      } else if (strcmp(type, @encode(double)) == 0) {
        body[stdKey] = EventValue([value doubleValue]);
      } else if (strcmp(type, @encode(float)) == 0) {
        body[stdKey] = EventValue([value floatValue]);
      } else if (strcmp(type, @encode(BOOL)) == 0) {
        body[stdKey] = EventValue([value boolValue]);
      }
    }
  }

  _eventHandler->invokeHandlerWithEventBody(name, body);
}

@end

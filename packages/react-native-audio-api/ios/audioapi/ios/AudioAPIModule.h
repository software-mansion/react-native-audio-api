#ifdef RCT_NEW_ARCH_ENABLED
#import <React/RCTCallInvokerModule.h>
#import <rnaudioapi/rnaudioapi.h>
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#import <React/RCTEventEmitter.h>

@class AudioEngine;
@class NotificationManager;
@class AudioSessionManager;
@class LockScreenManager;

@interface AudioAPIModule : RCTEventEmitter
#ifdef RCT_NEW_ARCH_ENABLED
                            <NativeAudioAPIModuleSpec, RCTCallInvokerModule>
#else
                            <RCTBridgeModule>
#endif // RCT_NEW_ARCH_ENABLED

@property (nonatomic, strong) AudioEngine *audioEngine;
@property (nonatomic, strong) NotificationManager *notificationManager;
@property (nonatomic, strong) AudioSessionManager *audioSessionManager;
@property (nonatomic, strong) LockScreenManager *lockScreenManager;

- (void)invokeHandlerWithEventName:(NSString *)eventName eventBody:(NSDictionary *)eventBody;

@end

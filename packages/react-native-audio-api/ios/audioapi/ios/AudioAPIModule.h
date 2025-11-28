#ifdef RCT_NEW_ARCH_ENABLED
#import <React/RCTCallInvokerModule.h>
#import <React/RCTInvalidating.h>
#import <rnaudioapi/rnaudioapi.h>
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#import <React/RCTEventEmitter.h>

@class AudioEngine;
@class NotificationManager;
@class AudioSessionManager;
@class NotificationRegistry;

@interface AudioAPIModule : RCTEventEmitter
#ifdef RCT_NEW_ARCH_ENABLED
                            <NativeAudioAPIModuleSpec, RCTCallInvokerModule, RCTInvalidating>
#else
                            <RCTBridgeModule>
#endif // RCT_NEW_ARCH_ENABLED

@property (nonatomic, strong) AudioEngine *audioEngine;
@property (nonatomic, strong) NotificationManager *notificationManager;
@property (nonatomic, strong) AudioSessionManager *audioSessionManager;
@property (nonatomic, strong) NotificationRegistry *notificationRegistry;

- (void)invokeHandlerWithEventName:(NSString *)eventName eventBody:(NSDictionary *)eventBody;

@end

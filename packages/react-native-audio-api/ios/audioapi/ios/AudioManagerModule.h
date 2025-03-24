#pragma once

#ifdef RCT_NEW_ARCH_ENABLED
#import <rnaudioapi/rnaudioapi.h>
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

@class AudioEngine;
@class NotificationManager;
@class AudioSessionManager;
@class LockScreenManager;

@interface AudioManagerModule : NativeAudioManagerModuleSpecBase
#ifdef RCT_NEW_ARCH_ENABLED
                                <NativeAudioManagerModuleSpec>
#else
                                <RCTBridgeModule>
#endif // RCT_NEW_ARCH_ENABLED

@property (nonatomic, strong) AudioEngine *audioEngine;
@property (nonatomic, strong) NotificationManager *notificationManager;
@property (nonatomic, strong) AudioSessionManager *audioSessionManager;
@property (nonatomic, strong) LockScreenManager *lockScreenManager;

@end

#pragma once

#ifdef RCT_NEW_ARCH_ENABLED
#import <rnaudioapi/rnaudioapi.h>
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#import <React/RCTEventEmitter.h>

@class AudioEngine;
@class NotificationManager;
@class AudioSessionManager;
@class LockScreenManager;

@interface AudioManagerModule :
#ifdef RCT_NEW_ARCH_ENABLED
    NativeAudioManagerModuleSpecBase <NativeAudioManagerModuleSpec>
#else
    RCTEventEmitter <RCTBridgeModule>
#endif // RCT_NEW_ARCH_ENABLED

@property (nonatomic, strong) AudioEngine *audioEngine;
@property (nonatomic, strong) NotificationManager *notificationManager;
@property (nonatomic, strong) AudioSessionManager *audioSessionManager;
@property (nonatomic, strong) LockScreenManager *lockScreenManager;

@end

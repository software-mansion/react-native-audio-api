#pragma once

#import <React/RCTBridgeModule.h>
#import <React/RCTEventEmitter.h>

@class AudioEngine;
@class NotificationManager;
@class AudioSessionManager;
@class LockScreenManager;

@interface AudioManagerModule : RCTEventEmitter <RCTBridgeModule>

@property (nonatomic, strong) AudioEngine *audioEngine;
@property (nonatomic, strong) NotificationManager *notificationManager;
@property (nonatomic, strong) AudioSessionManager *audioSessionManager;
@property (nonatomic, strong) LockScreenManager *lockScreenManager;

@end

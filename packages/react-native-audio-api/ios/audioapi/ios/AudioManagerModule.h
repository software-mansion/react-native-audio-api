#pragma once

#ifdef RCT_NEW_ARCH_ENABLED
#import <rnaudioapi/rnaudioapi.h>
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>
#import <audioapi/ios/system/AudioEngine.h>

@interface AudioManagerModule : NativeAudioManagerModuleSpecBase
#ifdef RCT_NEW_ARCH_ENABLED
                                <NativeAudioManagerModuleSpec>
#else
                                <RCTBridgeModule>
#endif // RCT_NEW_ARCH_ENABLED

@property (nonatomic, strong) AudioEngine *audioEngine;
@property (nonatomic, weak) AVAudioSession *audioSession;
@property (nonatomic, weak) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, assign) bool hadConfigurationChange;

@property (nonatomic, assign) AVAudioSessionMode sessionMode;
@property (nonatomic, assign) AVAudioSessionCategory sessionCategory;
@property (nonatomic, assign) AVAudioSessionCategoryOptions sessionOptions;
@property (nonatomic, copy) NSString *artworkUrl;

@end

#pragma once

#ifdef RCT_NEW_ARCH_ENABLED
#import <rnaudioapi/rnaudioapi.h>
#else // RCT_NEW_ARCH_ENABLED
#import <React/RCTBridgeModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#import <React/RCTEventEmitter.h>

#import <audioapi/ios/system/AudioManager.h>

@interface AudioManagerModule : RCTEventEmitter
#ifdef RCT_NEW_ARCH_ENABLED
                                <NativeAudioManagerModuleSpec>
#else
                                <RCTBridgeModule>
#endif // RCT_NEW_ARCH_ENABLED

@property (nonatomic, strong) AudioManager *audioManager;

@end

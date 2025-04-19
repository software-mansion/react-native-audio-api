#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioManagerModule;

@interface NotificationManager : NSObject

@property (nonatomic, weak) AudioManagerModule *audioManagerModule;
@property (nonatomic, weak) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, assign) bool hadConfigurationChange;
@property (nonatomic, assign) bool audioInterruptionsObserved;

+ (instancetype)sharedInstanceWithAudioManagerModule:(AudioManagerModule *)audioManagerModule;
- (void)cleanup;

- (void)observeAudioInterruptions:(BOOL)enabled;

@end

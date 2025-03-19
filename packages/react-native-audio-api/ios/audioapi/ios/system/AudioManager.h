#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>

#import <audioapi/ios/system/AudioEngine.h>

@interface AudioManager : NSObject

@property (nonatomic, strong) AudioEngine *audioEngine;
@property (nonatomic, weak) AVAudioSession *audioSession;
@property (nonatomic, weak) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, assign) bool hadConfigurationChange;

@property (nonatomic, assign) AVAudioSessionMode sessionMode;
@property (nonatomic, assign) AVAudioSessionCategory sessionCategory;
@property (nonatomic, assign) AVAudioSessionCategoryOptions sessionOptions;

- (instancetype)init;
- (void)cleanup;

- (float)getSampleRate;

- (void)setNowPlaying:(NSDictionary *)info;
- (void)setSessionOptionsWithCategory:(AVAudioSessionCategory)category
                                 mode:(AVAudioSessionMode)mode
                              options:(AVAudioSessionCategoryOptions)options;

@end

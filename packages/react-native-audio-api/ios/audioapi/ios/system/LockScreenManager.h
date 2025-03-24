#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>

@class AudioManagerModule;

@interface LockScreenManager : NSObject

@property (nonatomic, weak) AudioManagerModule *audioManagerModule;

@property (nonatomic, weak) MPNowPlayingInfoCenter *playingInfoCenter;
@property (nonatomic, copy) NSString *artworkUrl;

+ (instancetype)sharedInstanceWithAudioManagerModule:(AudioManagerModule *)audioManagerModule;
- (void)cleanup;

- (void)setLockScreenInfo:(NSDictionary *)info;
- (void)resetLockScreenInfo;

@end

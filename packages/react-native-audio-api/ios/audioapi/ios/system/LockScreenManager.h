#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>

@class AudioManagerModule;

@interface LockScreenManager : NSObject

@property (nonatomic, weak) AudioManagerModule *audioManagerModule;

@property (nonatomic, weak) MPNowPlayingInfoCenter *playingInfoCenter;
@property (nonatomic, copy) NSString *artworkUrl;

- (instancetype)initWithAudioManagerModule:(AudioManagerModule *)audioManagerModule;
- (void)cleanup;

- (void)setLockScreenInfo:(NSDictionary *)info;
- (void)resetLockScreenInfo;
- (void)enableRemoteCommand:(NSString *)name enabled:(BOOL)enabled;

@end

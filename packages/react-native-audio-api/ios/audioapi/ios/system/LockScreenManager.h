#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>

@class AudioAPIModule;

@interface LockScreenManager : NSObject

@property (nonatomic, weak) AudioAPIModule *audioAPIModule;

@property (nonatomic, weak) MPNowPlayingInfoCenter *playingInfoCenter;
@property (nonatomic, copy) NSString *artworkUrl;

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule;
- (void)cleanup;

- (void)setLockScreenInfo:(NSDictionary *)info;
- (void)resetLockScreenInfo;
- (void)enableRemoteCommand:(NSString *)name enabled:(BOOL)enabled;

@end

#pragma once

#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>
#import <audioapi/ios/system/notification/BaseNotification.h>

@class AudioAPIModule;

/**
 * PlaybackNotification
 *
 * iOS playback notification using MPNowPlayingInfoCenter and MPRemoteCommandCenter.
 * Provides lock screen controls, Control Center integration, and Now Playing display.
 *
 * Note: On iOS, this only manages metadata. Notification visibility is controlled
 * by the AudioContext state (active audio session shows controls).
 */
@interface PlaybackNotification : NSObject <BaseNotification>

@property (nonatomic, weak) AudioAPIModule *audioAPIModule;
@property (nonatomic, weak) MPNowPlayingInfoCenter *playingInfoCenter;
@property (nonatomic, copy) NSString *artworkUrl;
@property (nonatomic, assign) BOOL isActive;

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule;

@end

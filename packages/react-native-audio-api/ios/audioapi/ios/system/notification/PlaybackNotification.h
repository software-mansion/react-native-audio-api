#pragma once

#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>
#import <audioapi/ios/system/notification/ArtworkLoader.h>
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
 *
 * Every method must be called on the main queue, which owns all of its state. The artwork loader
 * must deliver there as well.
 */
@interface PlaybackNotification : NSObject <BaseNotification>

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule
                         artworkLoader:(ArtworkLoader *)artworkLoader NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

#pragma once


#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface AudioControls : NSObject

@property (nonatomic, weak) AVAudioSession *audioSession;
@property (nonatomic, weak) MPNowPlayingInfoCenter *nowPlayingInfoCenter;
@property (nonatomic, weak) MPRemoteCommandCenter *remoteCommandCenter;

- (instancetype)init;

@ned

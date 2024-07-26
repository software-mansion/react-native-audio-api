#pragma once

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface AudioContext : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;

- (instancetype)init;

@end

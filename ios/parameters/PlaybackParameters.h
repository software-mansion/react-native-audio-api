#pragma once
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface PlaybackParameters : NSObject

@property (nonatomic, assign) float *leftBuffer;
@property (nonatomic, assign) float *rightBuffer;
@property (nonatomic, assign) float leftGain;
@property (nonatomic, assign) float rightGain;
@property (nonatomic, assign) AVAudioFrameCount frameCount;

- (instancetype)initWithLeftBuffer:(float *)leftBuffer
                       rightBuffer:(float *)rightBuffer
                        frameCount:(AVAudioFrameCount)frameCount;

- (void)reset;

@end

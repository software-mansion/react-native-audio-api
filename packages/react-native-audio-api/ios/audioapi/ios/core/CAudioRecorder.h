#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

typedef void (^RenderAudioBlock)(AudioBufferList *outputBuffer, int numFrames);

@interface CAudioRecorder : NSObject

@property (nonatomic, assign) bool isRunning;
@property (nonatomic, assign) int bufferSize;
@property (nonatomic, strong) AVAudioNodeTapBlock tapBlock;

- (instancetype)init;

- (void)start;

- (void)stop;

- (void)cleanup;

@end

#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import "AudioContext.h"
#import "AudioScheduledSourceNode.h"

@interface AudioBufferSourceNode : AudioScheduledSourceNode

@property (nonatomic, assign) Boolean isPlaying;
@property (nonatomic, assign) bool loop;
//@property (nonatomic, assign) int bufferIndex;
//@property (nonatomic, strong) RNAA_AudioBuffer *buffer;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

- (void)setLoop:(bool)loop;

- (bool)getLoop;

@end

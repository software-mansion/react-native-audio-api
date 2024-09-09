#pragma once

#import "AudioContext.h"
#import "AudioScheduledSourceNode.h"
#import "RNAA_AudioBuffer.h"
#import "Constants.h"

@interface AudioBufferSourceNode : AudioScheduledSourceNode

@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, assign) Boolean isPlaying;
@property (nonatomic, strong) AVAudioSourceNode *sourceNode;
@property (nonatomic, assign) bool loop;
@property (nonatomic, assign) int bufferIndex;
@property (nonatomic, strong) RNAA_AudioBuffer *buffer;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

- (void)start:(double)time;

- (void)stop:(double)time;

- (void)setLoop:(bool)loop;

- (bool)getLoop;

@end

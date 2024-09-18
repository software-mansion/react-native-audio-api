#pragma once

#import "AudioScheduledSourceNode.h"
#import "Constants.h"
#import "RNAA_AudioBuffer.h"

@interface AudioBufferSourceNode : AudioScheduledSourceNode

@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, strong) AVAudioSourceNode *sourceNode;
@property (nonatomic, assign) bool loop;
@property (nonatomic, assign) int bufferIndex;
@property (nonatomic, strong) RNAA_AudioBuffer *buffer;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

- (void)setLoop:(bool)loop;

- (bool)getLoop;

@end

#pragma once

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "PlaybackParameters.h"

@class AudioContext;

@interface AudioNode : NSObject

@property (nonatomic, assign) int numberOfInputs;
@property (nonatomic, assign) int numberOfOutputs;
@property (nonatomic, strong) NSMutableArray<AudioNode *> *connectedNodes;
@property (nonatomic, strong) AudioContext *context;

- (instancetype)initWithContext:(AudioContext *)context;
- (void)cleanup;
- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList;
- (void)connect:(AudioNode *)node;
- (void)disconnect:(AudioNode *)node;
- (int)getNumberOfInputs;
- (int)getNumberOfOutputs;

@end

#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import "ChannelCountMode.h"
#import "ChannelInterpretation.h"

@class AudioContext;

@interface AudioNode : NSObject

@property (nonatomic, assign) int numberOfInputs;
@property (nonatomic, assign) int numberOfOutputs;
@property (nonatomic, assign) int channelCount;
@property (nonatomic, assign) ChannelCountModeEnum channelCountMode;
@property (nonatomic, assign) ChannelInterpretationEnum channelInterpretation;
@property (nonatomic, strong) NSMutableArray<AudioNode *> *connectedNodes;
@property (nonatomic, strong) AudioContext *context;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList;

- (void)connect:(AudioNode *)node;

- (void)disconnect:(AudioNode *)node;

- (int)getNumberOfInputs;

- (int)getNumberOfOutputs;

- (int)getChannelCount;

- (NSString *)getChannelCountMode;

- (NSString *)getChannelInterpretation;

@end

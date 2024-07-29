#pragma once

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@class AudioContext;

@interface AudioNode : NSObject

@property (nonatomic, assign) NSInteger numberOfInputs;
@property (nonatomic, assign) NSInteger numberOfOutputs;
@property (nonatomic, strong) NSMutableArray<AudioNode *> *connectedNodes;
@property (nonatomic, strong) AudioContext *context;

- (instancetype)init:(AudioContext *)context;
- (void)process:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode;
- (void)deprocess:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode nodeToDeprocess:(AudioNode *)node;
- (void)connect:(AudioNode *)node;
- (void)disconnect:(AudioNode *)node;

@end

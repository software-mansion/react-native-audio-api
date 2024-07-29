#pragma once

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "OscillatorNode.h"

@class OscillatorNode;

@interface AudioContext : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) NSMutableArray<OscillatorNode *> *connectedOscillators;

- (instancetype)init;

- (void)connectOscillator:(OscillatorNode *)node;

- (void)processNodes;

@end

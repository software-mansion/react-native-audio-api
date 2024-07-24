#import <Foundation/Foundation.h>
#import "AudioNode.h"
#import "OscillatorNode.h"

@interface GainNode : AudioNode

@property (nonatomic, assign) float gain;

- (instancetype)init;

- (void)changeGain:(double)gain;

@end

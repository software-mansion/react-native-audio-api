#import <Foundation/Foundation.h>
#import "AudioNode.h"
#import "OscillatorNode.h"

@interface GainNode : AudioNode

@property (nonatomic, assign) float gain;
@property (nonatomic, strong) NSMutableArray<AVAudioPlayerNode *> *playerNodes;

- (instancetype)init:(AudioContext *)context;

- (void)changeGain:(float)gain;

- (float)getGain;

@end

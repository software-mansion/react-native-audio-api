#import <Foundation/Foundation.h>
#import "AudioNode.h"
#import "OscillatorNode.h"

@interface GainNode : AudioNode

@property (nonatomic, assign) float gain;
@property (nonatomic, strong) NSMutableDictionary<NSUUID *, AVAudioEngine *> *connectedEngines;

- (instancetype)init;

- (void)changeGain:(double)gain;

- (double)getGain;

@end

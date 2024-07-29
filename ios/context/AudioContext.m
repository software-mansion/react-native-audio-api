#import <AudioContext.h>

@implementation AudioContext

- (instancetype)init {
    if (self != [super init]) {
      return self;
    }
    
    self.connectedOscillators = [NSMutableArray array];
    
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;

    return self;
}

- (void)connectOscillator:(OscillatorNode *)node {
    [self.connectedOscillators addObject:node];
}

- (void)processNodes {
    for (OscillatorNode *oscillator in self.connectedOscillators) {
        [oscillator process:oscillator.buffer playerNode:oscillator.playerNode];
    }
}

@end

#import <AudioContext.h>

@implementation AudioContext

- (instancetype)init {
    if (self != [super init]) {
      return self;
    }
    
    self.connectedOscillators = [NSMutableArray array];
    
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    
    _contextStartTime = mach_absolute_time();
    _state = ContextStateRunning;
    _sampleRate = 44100;

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

- (double)getCurrentTime {
    uint64_t currentTime = mach_absolute_time();
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);

    uint64_t elapsedNano = (currentTime - _contextStartTime) * info.numer / info.denom;
    return elapsedNano / 1e9;
}

- (NSString *)getState {
    return [ContextState toString:_state];
}

- (double)getSampleRate {
    return _sampleRate;
}

@end

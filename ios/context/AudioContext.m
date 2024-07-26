#import <AudioContext.h>

@implementation AudioContext

- (instancetype)init {
    if (self != [super init]) {
      return self;
    }
    
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;

    return self;
}

@end

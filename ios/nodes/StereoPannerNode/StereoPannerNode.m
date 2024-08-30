#import "StereoPannerNode.h"
#import "AudioContext.h"

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

@implementation StereoPannerNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self = [super initWithContext:context]) {
        _panParam = [[AudioParam alloc] initWithContext:context value:0 minValue:-1 maxValue:1];
        self.numberOfInputs = 1;
        self.numberOfOutputs = 1;
    }

    return self;
}

- (void)cleanup {
    _panParam = nil;
}

- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList; {
  float time = [self.context getCurrentTime];
  float deltaTime = 1 / self.context.sampleRate;

  float *bufferL = (float *)bufferList->mBuffers[0].mData;
  float *bufferR = (float *)bufferList->mBuffers[1].mData;

  for (int frame = 0; frame < frameCount; frame += 1) {
    double pan = [_panParam getValueAtTime:[self.context getCurrentTime]];
    double x = (pan + 1) / 2;

    bufferL[frame] *= cos(x * M_PI / 2);
    bufferR[frame] *= sin(x * M_PI / 2);

    time += deltaTime;
  }

  [super process:frameCount bufferList:bufferList];
};

@end

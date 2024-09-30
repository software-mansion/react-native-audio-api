#import "StereoPannerNode.h"
#import "AudioContext.h"

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

@implementation StereoPannerNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self = [super initWithContext:context]) {
    _panParam = [[AudioParam alloc] initWithContext:context value:0 minValue:-1 maxValue:1];
    self.numberOfInputs = 1;
    self.numberOfOutputs = 1;
    self.channelCountMode = ChannelCountModeClampedMax;
  }

  return self;
}

- (void)cleanup
{
  _panParam = nil;
}

- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList
{
  float time = [self.context getCurrentTime];
  float deltaTime = 1 / self.context.sampleRate;

  float *bufferL = (float *)bufferList->mBuffers[0].mData;
  float *bufferR = (float *)bufferList->mBuffers[1].mData;

  for (int frame = 0; frame < frameCount; frame += 1) {
    float pan = [_panParam getValueAtTime:[self.context getCurrentTime]];
    float x = ((pan <= 0) ? pan + 1 : pan) * M_PI / 2;

    float gainL = cos(x);
    float gainR = sin(x);

    float inputL = bufferL[frame];
    float inputR = bufferR[frame];

    if (pan <= 0) {
      bufferL[frame] += inputR * gainL;
      bufferR[frame] *= gainR;
    } else {
      bufferL[frame] *= gainL;
      bufferR[frame] += inputL * gainR;
    }

    time += deltaTime;
  }

  [super process:frameCount bufferList:bufferList];
};

@end

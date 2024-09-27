#import <AudioDestinationNode.h>
#import "AudioContext.h"
#import <math.h>

@implementation AudioDestinationNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self = [super initWithContext:context]) {
    self.numberOfInputs = INT_MAX;
    self.numberOfOutputs = 0;
    self.channelCountMode = ChannelCountModeExplicit;
  }

  return self;
}

- (void)normalize:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList {
  float *bufferL = (float *)bufferList->mBuffers[0].mData;
  float *bufferR = (float *)bufferList->mBuffers[1].mData;

  float maxL = 1;
  float maxR = 1;

  for (int frame = 0; frame < frameCount; frame += 1) {
    maxL = fmaxf(maxL,fabsf(bufferL[frame]));
    maxR = fmaxf(maxR,fabsf(bufferR[frame]));
  }

  for (int frame = 0; frame < frameCount; frame += 1) {
    bufferL[frame] /= maxL;
    bufferR[frame] /= maxR;
  }
}

- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList
{
  [self normalize:frameCount bufferList:bufferList];
}

@end

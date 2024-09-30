#import <GainNode.h>
#import "AudioContext.h"
#import "Constants.h"

@implementation GainNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self = [super initWithContext:context]) {
    _gainParam = [[AudioParam alloc] initWithContext:context
                                               value:1
                                            minValue:-[Constants maxGain]
                                            maxValue:[Constants maxGain]];
    self.numberOfInputs = 1;
    self.numberOfOutputs = 1;
  }

  return self;
}

- (void)cleanup
{
  _gainParam = nil;
}

- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList;
{
  float time = [self.context getCurrentTime];
  float deltaTime = 1 / self.context.sampleRate;

  for (int frame = 0; frame < frameCount; frame += 1) {
    float currentGain = [_gainParam getValueAtTime:time];

    for (int bufferNum = 0; bufferNum < bufferList->mNumberBuffers; bufferNum += 1) {
      float *buffer = (float *)bufferList->mBuffers[bufferNum].mData;

      buffer[frame] *= currentGain;
    }

    time += deltaTime;
  }

  [super process:frameCount bufferList:bufferList];
}

@end

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <PlaybackParameters.h>

@implementation PlaybackParameters

- (instancetype)initWithLeftBuffer:(float *)leftBuffer
                       rightBuffer:(float *)rightBuffer
                        frameCount:(AVAudioFrameCount)frameCount
{
  self = [super init];
  if (self) {
    _leftBuffer = leftBuffer;
    _rightBuffer = rightBuffer;
    _frameCount = frameCount;
    _leftGain = 1.0f;
    _rightGain = 1.0f;
  }
  return self;
}

- (void)reset
{
  _leftGain = 1.0f;
  _rightGain = 1.0f;
}

@end

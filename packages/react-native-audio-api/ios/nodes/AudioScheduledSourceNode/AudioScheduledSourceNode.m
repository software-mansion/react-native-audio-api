#import "AudioScheduledSourceNode.h"
#import "AudioContext.h"

@implementation AudioScheduledSourceNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self = [super initWithContext:context]) {
    _isPlaying = NO;
  }

  return self;
}

- (void)start:(double)time
{
  if (_isPlaying) {
    return;
  }

  double delay = time - [self.context getCurrentTime];
  if (delay <= 0) {
    [self startPlayback];
  } else {
    [NSTimer scheduledTimerWithTimeInterval:delay
                                     target:self
                                   selector:@selector(startPlayback)
                                   userInfo:nil
                                    repeats:NO];
  }
}

- (void)startPlayback
{
  _isPlaying = YES;
}

- (void)stop:(double)time
{
  if (!_isPlaying) {
    return;
  }

  double delay = time - [self.context getCurrentTime];
  if (delay <= 0) {
    [self stopPlayback];
  } else {
    [NSTimer scheduledTimerWithTimeInterval:delay target:self selector:@selector(stopPlayback) userInfo:nil repeats:NO];
  }
}

- (void)stopPlayback
{
  _isPlaying = NO;
}

@end

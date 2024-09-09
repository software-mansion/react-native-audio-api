#import <AudioBufferSourceNode.h>
#import "Constants.h"

@implementation AudioBufferSourceNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self != [super initWithContext:context]) {
    return self;
  }

  _loop = true;
  _isPlaying = NO;

  self.numberOfOutputs = 1;
  self.numberOfInputs = 0;

  return self;
}

- (void)cleanup
{
}

- (bool)getLoop
{
  return _loop;
}

- (void)setLoop:(bool)loop
{
  _loop = loop;
}

@end

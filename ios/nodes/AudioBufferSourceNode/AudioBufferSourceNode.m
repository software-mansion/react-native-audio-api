#import <AudioBufferSourceNode.h>
#import "Constants.h"

@implementation AudioBufferSourceNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self = [super initWithContext:context]) {
      self.numberOfOutputs = 1;
      self.numberOfInputs = 0;
      self.loop = true;
      self.isPlaying = true;
  }

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

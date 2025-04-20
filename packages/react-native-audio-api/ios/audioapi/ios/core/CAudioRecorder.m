#import <audioapi/ios/core/CAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation CAudioRecorder

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
{
  if (self = [super init]) {
    self.tapId = nil;
    self.receiverBlock = [receiverBlock copy];

    __weak typeof(self) weakSelf = self;
    self.tapBlock = ^(AVAudioPCMBuffer *buffer, AVAudioTime *when) {
      if (!weakSelf.isRunning) {
        return;
      }

      weakSelf.receiverBlock(buffer, buffer.frameLength, when);
    };
  }

  return self;
}

- (void)start
{
  self.tapId = [[AudioEngine sharedInstance] installInputTap:self.tapBlock];
  self.isRunning = true;
}

- (void)stop
{
  self.isRunning = false;
  [[AudioEngine sharedInstance] removeInputTap:self.tapId];
}

- (void)cleanup
{
  if (self.isRunning) {
    [self stop];
  }
}

@end

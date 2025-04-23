#import <audioapi/ios/core/NativeIOSAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation NativeIOSAudioRecorder

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
                         bufferLength:(int)bufferLength
                enableVoiceProcessing:(bool)enableVoiceProcessing
{
  if (self = [super init]) {
    self.tapId = nil;
    self.isRunning = false;

    self.bufferLength = bufferLength;
    self.enableVoiceProcessing = enableVoiceProcessing;

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
  self.tapId = [[AudioEngine sharedInstance] installInputTap:self.tapBlock
                                                bufferLength:self.bufferLength
                                       enableVoiceProcessing:self.enableVoiceProcessing];
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

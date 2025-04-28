#import <audioapi/ios/core/NativeIOSAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation NativeIOSAudioRecorder

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock bufferLength:(int)bufferLength
{
  if (self = [super init]) {
    self.bufferLength = bufferLength;

    self.receiverBlock = [receiverBlock copy];

    __weak typeof(self) weakSelf = self;
    self.receiverSinkBlock = ^OSStatus(
        const AudioTimeStamp *_Nonnull timestamp,
        AVAudioFrameCount frameCount,
        const AudioBufferList *_Nonnull inputData) {
      AVAudioTime *time = [[AVAudioTime alloc] initWithAudioTimeStamp:timestamp sampleRate:48000];
      weakSelf.receiverBlock(inputData, frameCount, time);

      return kAudioServicesNoError;
    };

    self.sinkNode = [[AVAudioSinkNode alloc] initWithReceiverBlock:self.receiverSinkBlock];
  }

  return self;
}

- (void)start
{
  [[AudioEngine sharedInstance] attachInputNode:self.sinkNode];
}

- (void)stop
{
  [[AudioEngine sharedInstance] detachInputNode];
}

- (void)cleanup
{
  self.receiverBlock = nil;
}

@end

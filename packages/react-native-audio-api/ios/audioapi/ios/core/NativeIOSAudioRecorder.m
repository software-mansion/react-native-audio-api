#import <audioapi/ios/core/NativeIOSAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation NativeIOSAudioRecorder

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
                         bufferLength:(int)bufferLength
                enableVoiceProcessing:(bool)enableVoiceProcessing
{
  if (self = [super init]) {
    self.isRunning = false;

    self.bufferLength = bufferLength;
    self.enableVoiceProcessing = enableVoiceProcessing;

    self.receiverBlock = [receiverBlock copy];

    __weak typeof(self) weakSelf = self;
    self.receiverSinkBlock = ^OSStatus(const AudioTimeStamp * _Nonnull timestamp, AVAudioFrameCount frameCount, const AudioBufferList * _Nonnull inputData){
      if (!weakSelf.isRunning) {
        AVAudioTime *time = [[AVAudioTime alloc] initWithAudioTimeStamp:timestamp sampleRate:48000];
        weakSelf.receiverBlock(inputData, frameCount, time);
      }
      
      return kAudioServicesNoError;
    };
    
    self.sinkNode = [[AVAudioSinkNode alloc] initWithReceiverBlock:self.receiverSinkBlock];
  }

  return self;
}

- (void)start
{
  [[AudioEngine sharedInstance] attachInputNode:self.sinkNode];
  self.isRunning = true;
}

- (void)stop
{
  self.isRunning = false;
  [[AudioEngine sharedInstance] detachInputNode];
}

- (void)cleanup
{
  if (self.isRunning) {
    [self stop];
  }
}

@end

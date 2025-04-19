#import <audioapi/ios/core/CAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation CAudioRecorder

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
{
  if (self = [super init]) {
    NSLog(@"[AudioRecorder] init");
    self.receiverBlock = [receiverBlock copy];

    __weak typeof(self) weakSelf = self;
    self.tapBlock = ^(AVAudioPCMBuffer *buffer, AVAudioTime *when) {
      weakSelf.receiverBlock(buffer, buffer.frameLength, when);
    };
  }

  return self;
}

- (void)start
{
  NSLog(@"[AudioRecorder] start");
  NSError *error;
  AudioEngine *audioEngine = [AudioEngine sharedInstance];

  AVAudioInputNode *input = [audioEngine.audioEngine inputNode];
  [audioEngine.audioEngine prepare];
  [audioEngine.audioEngine startAndReturnError:&error];

  AVAudioFormat *format = [input outputFormatForBus:0];
  NSLog(@"format: %@", format);
  [input installTapOnBus:0 bufferSize:2048 format:format block:_tapBlock];

  self.isRunning = true;
}

- (void)stop
{
  NSLog(@"[AudioRecorder] stop");
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  AVAudioInputNode *input = [audioEngine.audioEngine inputNode];

  [input removeTapOnBus:0];
  self.isRunning = false;
}

- (void)cleanup
{
  NSLog(@"[AudioRecorder] cleanup");
  if ([self isRunning]) {
    [self stop];
  }
}

@end

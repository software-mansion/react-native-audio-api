#import <audioapi/ios/core/CAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation CAudioRecorder

- (instancetype)init
{
  if (self = [super init]) {
    NSLog(@"[AudioRecorder] init");

    self.tapBlock = ^(AVAudioPCMBuffer *_Nonnull buffer, AVAudioTime *_Nonnull when) {
      NSLog(@"tap taap: %@ %d", buffer.format, buffer.frameLength);
    };
  }

  return self;
}

- (void)start
{
  NSLog(@"[AudioRecorder] start");
  NSError *error;
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  AudioSessionManager *manager = [AudioSessionManager sharedInstance];

  [manager.audioSession setPreferredSampleRate:48000 error:&error];

  AVAudioInputNode *input = [audioEngine.audioEngine inputNode];

  AVAudioFormat *format = [input outputFormatForBus:0];
  [audioEngine.audioEngine prepare];

  NSLog(@"format: %@", format);

  [input installTapOnBus:0 bufferSize:8192 format:format block:_tapBlock];

  [audioEngine startEngine];
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

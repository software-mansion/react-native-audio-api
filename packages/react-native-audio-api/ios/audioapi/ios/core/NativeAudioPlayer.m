#import <audioapi/ios/core/NativeAudioPlayer.h>
#import <audioapi/ios/system/AudioEngine.h>

@implementation NativeAudioPlayer

- (instancetype)initWithRenderAudio:(RenderAudioBlock)renderAudio
                         sampleRate:(float)sampleRate
                       channelCount:(int)channelCount
{
  if (self = [super init]) {
    self.sampleRate = sampleRate;

    self.channelCount = channelCount;
    self.renderAudio = [renderAudio copy];

    __weak typeof(self) weakSelf = self;
    self.renderBlock = ^OSStatus(
        BOOL *isSilence, const AudioTimeStamp *timestamp, AVAudioFrameCount frameCount, AudioBufferList *outputData) {
      if (outputData->mNumberBuffers != weakSelf.channelCount) {
        return kAudioServicesBadPropertySizeError;
      }

      weakSelf.renderAudio(outputData, frameCount);

      return kAudioServicesNoError;
    };

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:self.channelCount];
    _sourceNode = [[AVAudioSourceNode alloc] initWithFormat:self.format renderBlock:self.renderBlock];
  }

  return self;
}

- (void)start
{
  NSLog(@"[AudioPlayer] start");

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);
  self.sourceNodeId = [audioEngine attachSourceNode:self.sourceNode format:self.format];
}

- (void)stop
{
  NSLog(@"[AudioPlayer] stop");

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);
  [audioEngine detachSourceNodeWithId:self.sourceNodeId];
  self.sourceNodeId = nil;
}

- (void)resume
{
  NSLog(@"[AudioPlayer] resume");
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);
  [audioEngine startEngine];
}

- (void)suspend
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);
  [audioEngine pauseEngine:self.sourceNodeId];
}

- (void)cleanup
{
  self.renderAudio = nil;
}

@end

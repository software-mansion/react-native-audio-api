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

- (bool)start
{
  NSLog(@"[AudioPlayer] start");

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  // AudioEngine allows us to attach and connect nodes at runtime but with few limitations
  // in this case if it is the first player and recorder started the engine we need to restart.
  // It can be optimized by tracking if we haven't break rules of at runtime modifications from docs
  // https://developer.apple.com/documentation/avfaudio/avaudioengine?language=objc
  //
  // Currently we are restarting because we do not see any significant performance issue and case when
  // you will need to start and stop player very frequently
  [audioEngine stopEngine];
  self.sourceNodeId = [audioEngine attachSourceNode:self.sourceNode format:self.format];
  return [audioEngine startIfNecessary];
}

- (void)stop
{
  NSLog(@"[AudioPlayer] stop");

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);
  [audioEngine detachSourceNodeWithId:self.sourceNodeId];
  [audioEngine stopIfNecessary];
  self.sourceNodeId = nil;
}

- (bool)resume
{
  NSLog(@"[AudioPlayer] resume");
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  return [audioEngine startIfNecessary];
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

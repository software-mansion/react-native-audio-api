#import <audioapi/ios/core/NativeAudioPlayer.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation NativeAudioPlayer

- (void)detachSourceNodeIfAttached:(AudioEngine *)audioEngine
{
  if (self.sourceNodeId == nil) {
    return;
  }

  [audioEngine detachSourceNodeWithId:self.sourceNodeId];
  self.sourceNodeId = nil;
}

- (void)attachSourceNodeIfNeeded:(AudioEngine *)audioEngine
{
  if (self.sourceNodeId != nil) {
    return;
  }

  self.sourceNodeId = [audioEngine attachSourceNodeWithRenderBlock:self.renderBlock
                                                        sampleRate:self.sampleRate
                                                      channelCount:self.channelCount];
}

- (bool)startPlaybackGraph:(AudioEngine *)audioEngine
{
  [audioEngine stopIfNecessary];
  [self attachSourceNodeIfNeeded:audioEngine];
  return [audioEngine startIfNecessary];
}

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
        BOOL *isSilence,
        const AudioTimeStamp *timestamp,
        AVAudioFrameCount frameCount,
        AudioBufferList *outputData) {
      if (outputData->mNumberBuffers != weakSelf.channelCount) {
        return kAudioServicesBadPropertySizeError;
      }

      weakSelf.renderAudio(outputData, frameCount);

      return kAudioServicesNoError;
    };
  }

  return self;
}

- (bool)start
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];
  assert(audioEngine != nil);

  NSError *error = nil;
  if (![sessionManager ensureActive:false error:&error]) {
    NSLog(@"Error while activating audio session for playback: %@", [error debugDescription]);
    return false;
  }

  // AudioEngine allows us to attach and connect nodes at runtime but with few
  // limitations in this case if it is the first player and recorder started the
  // engine we need to restart. It can be optimized by tracking if we haven't
  // break rules of at runtime modifications from docs
  // https://developer.apple.com/documentation/avfaudio/avaudioengine?language=objc
  //
  // Currently we are restarting because we do not see any significant performance issue and case when
  // you will need to start and stop player very frequently
  return [self startPlaybackGraph:audioEngine];
}

- (void)stop
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  [self detachSourceNodeIfAttached:audioEngine];
  [audioEngine stopIfPossible];
}

- (bool)resume
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];
  assert(audioEngine != nil);

  NSError *error = nil;
  if (![sessionManager ensureActive:false error:&error]) {
    NSLog(@"Error while re-activating audio session for playback: %@", [error debugDescription]);
    return false;
  }

  return [self startPlaybackGraph:audioEngine];
}

- (void)suspend
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  [self detachSourceNodeIfAttached:audioEngine];
  [audioEngine stopIfPossible];
}

- (void)cleanup
{
  self.renderAudio = nil;
  self.renderBlock = nil;
}

@end

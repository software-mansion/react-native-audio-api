#import <AudioPlayer.h>

@implementation AudioPlayer

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio
{
  if (self = [super init]) {
    self.renderAudio = [renderAudio copy];
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;

    self.audioSession = AVAudioSession.sharedInstance;
    NSError *error = nil;

    // TODO:
    // We will probably want to change it to AVAudioSessionCategoryPlayAndRecord in the future.
    // Eventually we to make this a dynamic setting, if user of the lib wants to use recording features.
    // But setting a recording category might require some setup first, so lets skip it for now :)
    [self.audioSession setCategory:AVAudioSessionCategoryPlayback error:&error];

    if (error != nil) {
      @throw error;
    }

    [self.audioSession setActive:true error:&error];

    if (error != nil) {
      @throw error;
    }

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:[self.audioSession sampleRate] channels:2];

    __weak typeof(self) weakSelf = self;
    _sourceNode = [[AVAudioSourceNode alloc] initWithFormat:self.format
                                                renderBlock:^OSStatus(
                                                    BOOL *isSilence,
                                                    const AudioTimeStamp *timestamp,
                                                    AVAudioFrameCount frameCount,
                                                    AudioBufferList *outputData) {
                                                  return [weakSelf renderCallbackWithIsSilence:isSilence
                                                                                     timestamp:timestamp
                                                                                    frameCount:frameCount
                                                                                    outputData:outputData];
                                                }];
  }

  return self;
}

- (int)getSampleRate
{
  return [self.audioSession sampleRate];
}

- (int)getBufferSizeInFrames
{
  // Note: might be important in the future.
  // For some reason audioSession.IOBufferDuration is always 0.01, which for sample rate of 48k
  // gives exactly 480 frames, while at the same time frameCount requested by AVAudioSourceEngine
  // might vary f.e. between 555-560.
  // preferredIOBufferDuration seems to be double the value (resulting in 960 frames),
  // which is safer to base our internal AudioBus sizes.
  // Buut no documentation => no guarantee :)
  // If something is crackling when it should play silence, start here 📻
  double maxBufferDuration =
      fmax(0.02, fmax(self.audioSession.IOBufferDuration, self.audioSession.preferredIOBufferDuration));
  return (int)(maxBufferDuration * self.audioSession.sampleRate + 1);
}

- (void)start
{
  [self.audioEngine attachNode:self.sourceNode];
  [self.audioEngine connect:self.sourceNode to:self.audioEngine.mainMixerNode format:self.format];

  if (!self.audioEngine.isRunning) {
    NSError *error = nil;
    if (![self.audioEngine startAndReturnError:&error]) {
      NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
    }
  }
}

- (void)stop
{
  [self.audioEngine detachNode:self.sourceNode];

  if (self.audioEngine.isRunning) {
    [self.audioEngine stop];
  }

  NSError *error = nil;
  [self.audioSession setActive:false error:&error];

  if (error != nil) {
    @throw error;
  }
}

- (void)cleanup
{
  self.audioEngine = nil;
  self.audioSession = nil;
  self.renderAudio = nil;
}

- (OSStatus)renderCallbackWithIsSilence:(BOOL *)isSilence
                              timestamp:(const AudioTimeStamp *)timestamp
                             frameCount:(AVAudioFrameCount)frameCount
                             outputData:(AudioBufferList *)outputData
{
  if (outputData->mNumberBuffers < 2) {
    return noErr; // Ensure we have stereo output
  }

  self.renderAudio(outputData, frameCount);

  return noErr;
}

@end

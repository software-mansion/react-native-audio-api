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

    self.sampleRate = [self.audioSession sampleRate];

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:2];

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

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio sampleRate:(float)sampleRate
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

    self.sampleRate = sampleRate;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:2];

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

- (float)getSampleRate
{
  return self.sampleRate;
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

#import <audioapi/ios/core/AudioPlayer.h>

@implementation AudioPlayer

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio channelCount:(int)channelCount
{
  if (self = [super init]) {
    self.renderAudio = [renderAudio copy];
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    self.isRunning = true;

    [self setupAndInitAudioSession];
    [self setupAndInitNotificationHandlers];

    self.sampleRate = [self.audioSession sampleRate];
    self.channelCount = channelCount;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:self.channelCount];

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

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio
                              sampleRate:(float)sampleRate
                            channelCount:(int)channelCount
{
  if (self = [super init]) {
    self.renderAudio = [renderAudio copy];
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    self.isRunning = true;

    [self setupAndInitAudioSession];
    [self setupAndInitNotificationHandlers];

    self.sampleRate = sampleRate;
    self.channelCount = channelCount;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:self.channelCount];

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
  self.isRunning = true;
  [self connectAudioEngine];
}

- (void)stop
{
  self.isRunning = false;
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

- (void)suspend
{
  [self.audioEngine pause];
  self.isRunning = false;
}

- (void)resume
{
  self.isRunning = true;
  [self setupAndInitAudioSession];
  [self connectAudioEngine];
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
  if (outputData->mNumberBuffers < self.channelCount) {
    return noErr; // Ensure we have stereo output
  }

  //  NSLog(@"sampleRate: %f %d", self.audioSession.sampleRate, frameCount);

  self.renderAudio(outputData, frameCount);

  return noErr;
}

- (void)setupAndInitAudioSession
{
  NSError *error = nil;

  if (!self.audioSession) {
    self.audioSession = [AVAudioSession sharedInstance];
  }

  NSLog(@"available categories: %@", [self.audioSession availableCategories]);
  NSLog(@"available modes: %@", [self.audioSession availableModes]);
  //  NSLog(@"available options: %@", [self.audioSession])

  //  if (self.sampleRate != 0) {
  //    [sle]
  //  }

  [self.audioSession setPreferredIOBufferDuration:0.01 error:&error];

  [self.audioSession setCategory:AVAudioSessionCategoryPlayback
                            mode:AVAudioSessionModeDefault
                         options:AVAudioSessionCategoryOptionDuckOthers
                           error:&error];

  if (error != nil) {
    NSLog(@"Error while configuring audio session: %@", [error debugDescription]);
  }

  [self.audioSession setActive:true error:&error];

  if (error != nil) {
    NSLog(@"Error while activating audio session: %@", [error debugDescription]);
  }
}

- (void)setupAndInitNotificationHandlers
{
  if (!self.notificationCenter) {
    self.notificationCenter = [NSNotificationCenter defaultCenter];
  }

  [self.notificationCenter addObserver:self
                              selector:@selector(handleEngineConfigurationChange:)
                                  name:AVAudioEngineConfigurationChangeNotification
                                object:nil];
}

- (void)connectAudioEngine
{
  if ([self.audioEngine isRunning]) {
    return;
  }

  [self.audioEngine detachNode:self.sourceNode];

  _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:self.channelCount];

  __weak typeof(self) weakSelf = self;
  self.sourceNode = [[AVAudioSourceNode alloc] initWithFormat:self.format
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

  [self.audioEngine attachNode:self.sourceNode];
  [self.audioEngine connect:self.sourceNode to:self.audioEngine.mainMixerNode format:self.format];

  if (![self.audioEngine isRunning]) {
    NSError *error = nil;

    if (![self.audioEngine startAndReturnError:&error]) {
      NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
    }
  }
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  if (!self.isRunning) {
    self.hadAudioConfigurationChange = true;
    return;
  }

  if (self.isInterrupted) {
    self.hadAudioConfigurationChange = true;
    return;
  }

  dispatch_async(dispatch_get_main_queue(), ^{
    [self connectAudioEngine];
  });
}

- (void)handleInterruption:(NSNotification *)notification
{
  NSError *error;
  UInt8 type = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
  UInt8 option = [[notification.userInfo valueForKey:AVAudioSessionInterruptionOptionKey] intValue];

  if (type == AVAudioSessionInterruptionTypeBegan) {
    self.isInterrupted = true;
    return;
  }

  if (type != AVAudioSessionInterruptionTypeEnded) {
    return;
  }

  if (option != AVAudioSessionInterruptionOptionShouldResume) {
    return;
  }

  bool success = [self.audioSession setActive:true error:&error];

  if (!success) {
    NSLog(@"ERror: %@", [error debugDescription]);
    return;
  }

  if (self.hadAudioConfigurationChange) {
    [self connectAudioEngine];
  }
}

@end

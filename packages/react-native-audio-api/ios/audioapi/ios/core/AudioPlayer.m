#import <audioapi/ios/core/AudioPlayer.h>

@implementation AudioPlayer

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio
{
  if (self = [super init]) {
    self.renderAudio = [renderAudio copy];
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    self.isRunning = true;

    [self setupAndInitAudioSession];

    self.sampleRate = [self.audioSession sampleRate];

    __weak typeof(self) weakSelf = self;
    self.renderBlock = ^OSStatus(
        BOOL *isSilence, const AudioTimeStamp *timestamp, AVAudioFrameCount frameCount, AudioBufferList *outputData) {
      if (outputData->mNumberBuffers != 2) {
        return kAudioServicesBadPropertySizeError;
      }

      weakSelf.renderAudio(outputData, frameCount);
      return kAudioServicesNoError;
    };

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:2];
    _sourceNode = [[AVAudioSourceNode alloc] initWithFormat:self.format renderBlock:self.renderBlock];
  }

  return self;
}

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio sampleRate:(float)sampleRate
{
  if (self = [super init]) {
    self.renderAudio = [renderAudio copy];
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    self.isRunning = true;

    [self setupAndInitAudioSession];

    self.sampleRate = sampleRate;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:2];

    __weak typeof(self) weakSelf = self;
    self.renderBlock = ^OSStatus(
        BOOL *isSilence, const AudioTimeStamp *timestamp, AVAudioFrameCount frameCount, AudioBufferList *outputData) {
      if (outputData->mNumberBuffers != 2) {
        return kAudioServicesBadPropertySizeError;
      }

      weakSelf.renderAudio(outputData, frameCount);
      return kAudioServicesNoError;
    };

    _sourceNode = [[AVAudioSourceNode alloc] initWithFormat:self.format renderBlock:self.renderBlock];
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

- (void)setupAndInitAudioSession
{
  NSError *error = nil;

  if (!self.audioSession) {
    self.audioSession = [AVAudioSession sharedInstance];
  }

  [self.audioSession setCategory:AVAudioSessionCategoryPlayback
                            mode:AVAudioSessionModeDefault
                         options:AVAudioSessionCategoryOptionDuckOthers | AVAudioSessionCategoryOptionAllowBluetooth |
                         AVAudioSessionCategoryOptionAllowAirPlay
                           error:&error];

  if (error != nil) {
    NSLog(@"Error while configuring audio session: %@", [error localizedDescription]);
  }

  [self.audioSession setActive:true error:&error];

  if (error != nil) {
    NSLog(@"Error while activating audio session: %@", [error localizedDescription]);
  }
}

- (void)connectAudioEngine
{
  if ([self.audioEngine isRunning]) {
    return;
  }

  [self.audioEngine attachNode:self.sourceNode];
  [self.audioEngine connect:self.sourceNode to:self.audioEngine.mainMixerNode format:self.format];

  if (![self.audioEngine isRunning]) {
    NSError *error = nil;

    if (![self.audioEngine startAndReturnError:&error]) {
      NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
    }
  }
}

@end

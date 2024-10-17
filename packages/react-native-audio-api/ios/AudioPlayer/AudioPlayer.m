#import <AudioPlayer.h>

@implementation AudioPlayer

- (instancetype)init
{
  if (self = [super init]) {
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    self.destination = [[AudioDestinationNode alloc] initWithContext:self];

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
    _sourceNode = [[AVAudioSourceNode alloc] initWithFormat:_format
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

    if (!self.audioEngine.isRunning) {
      NSError *error = nil;
      if (![self.audioEngine startAndReturnError:&error]) {
        NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
      }
    }
  }

  return self;
}

- (int)getSampleRate
{
  return [self.audioSession sampleRate];
}

- (int)getFramesPerBurst
{
  return 1;
}

- (void)start
{
  //
}

- (void)stop
{
  [self.audioEngine detachNode:self.sourceNode];
  
  NSError *error = nil;
  [self.audioSession setActive:false error:&error];

  if (error != nil) {
    @throw error;
  }

  self.audioSession = nil;

  if (self.audioEngine.isRunning) {
    [self.audioEngine stop];
  }

  self.audioEngine = nil;
}

- (OSStatus)renderCallbackWithIsSilence:(BOOL *)isSilence
                              timestamp:(const AudioTimeStamp *)timestamp
                             frameCount:(AVAudioFrameCount)frameCount
                             outputData:(AudioBufferList *)outputData
{
  if (outputData->mNumberBuffers < 2) {
    return noErr; // Ensure we have stereo output
  }

  float *leftBuffer = (float *)outputData->mBuffers[0].mData;
  float *rightBuffer = (float *)outputData->mBuffers[1].mData;

  return noErr;
}

@end


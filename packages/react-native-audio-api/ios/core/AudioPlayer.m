#import <AudioPlayer.h>

@implementation AudioPlayer

- (instancetype)initWithAudioManager:(IOSAudioManager *)audioManager renderAudio:(RenderAudioBlock)renderAudio
{
  if (self = [super init]) {
  }

  return self;
}

- (instancetype)initWithAudioManager:(IOSAudioManager *)audioManager
                         renderAudio:(RenderAudioBlock)renderAudio
                          sampleRate:(float)sampleRate
{
  if (self = [super init]) {
  }

  return self;
}

//- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio
//{
//  if (self = [super init]) {
//    self.renderAudio = [renderAudio copy];
//    self.audioEngine = [[AVAudioEngine alloc] init];
//    self.audioEngine.mainMixerNode.outputVolume = 1;
//    self.isRunning = true;
//
//    [self setupAndInitAudioSession];
//    [self setupAndInitNotificationHandlers];
//
//    self.sampleRate = [self.audioSession sampleRate];
//
//    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:2];
//
//    __weak typeof(self) weakSelf = self;
//    _sourceNode = [[AVAudioSourceNode alloc] initWithFormat:self.format
//                                                renderBlock:^OSStatus(
//                                                    BOOL *isSilence,
//                                                    const AudioTimeStamp *timestamp,
//                                                    AVAudioFrameCount frameCount,
//                                                    AudioBufferList *outputData) {
//                                                  return [weakSelf renderCallbackWithIsSilence:isSilence
//                                                                                     timestamp:timestamp
//                                                                                    frameCount:frameCount
//                                                                                    outputData:outputData];
//                                                }];
//  }
//
//  return self;
//}
//
//- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio sampleRate:(float)sampleRate
//{
//  if (self = [super init]) {
//    self.renderAudio = [renderAudio copy];
//    self.audioEngine = [[AVAudioEngine alloc] init];
//    self.audioEngine.mainMixerNode.outputVolume = 1;
//    self.isRunning = true;
//
//    [self setupAndInitAudioSession];
//    [self setupAndInitNotificationHandlers];
//
//    self.sampleRate = sampleRate;
//
//    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:2];
//
//    __weak typeof(self) weakSelf = self;
//    _sourceNode = [[AVAudioSourceNode alloc] initWithFormat:self.format
//                                                renderBlock:^OSStatus(
//                                                    BOOL *isSilence,
//                                                    const AudioTimeStamp *timestamp,
//                                                    AVAudioFrameCount frameCount,
//                                                    AudioBufferList *outputData) {
//                                                  return [weakSelf renderCallbackWithIsSilence:isSilence
//                                                                                     timestamp:timestamp
//                                                                                    frameCount:frameCount
//                                                                                    outputData:outputData];
//                                                }];
//  }
//
//  return self;
//}

- (float)getSampleRate
{
  return self.sampleRate;
}

- (void)start
{
  //  self.isRunning = true;
  //  [self connectAudioEngine];
}

- (void)stop
{
  //  self.isRunning = false;
  //  [self.audioEngine detachNode:self.sourceNode];
  //
  //  if (self.audioEngine.isRunning) {
  //    [self.audioEngine stop];
  //  }
  //
  //  NSError *error = nil;
  //  [self.audioSession setActive:false error:&error];
  //
  //  if (error != nil) {
  //    @throw error;
  //  }
}

- (void)suspend
{
  //  [self.audioEngine pause];
  //  self.isRunning = false;
}

- (void)resume
{
  //  self.isRunning = true;
  //  [self setupAndInitAudioSession];
  //  [self connectAudioEngine];
}

- (void)cleanup
{
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

#import <AudioPlayer.h>

@implementation AudioPlayer

- (instancetype)initWithAudioManager:(IOSAudioManager *)audioManager renderAudio:(RenderAudioBlock)renderAudio
{
  if (self = [super init]) {
    self.renderAudio = [renderAudio copy];
    self.audioManager = audioManager;
    self.sampleRate = [self.audioManager getSampleRate];

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

- (instancetype)initWithAudioManager:(IOSAudioManager *)audioManager
                         renderAudio:(RenderAudioBlock)renderAudio
                          sampleRate:(float)sampleRate
{
  if (self = [super init]) {
    self.renderAudio = [renderAudio copy];
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
  self.isRunning = true;
  self.sourceNodeId = [self.audioManager attachSourceNode:self.sourceNode format:self.format];
}

- (void)stop
{
  self.isRunning = false;
  [self.audioManager detachSourceNodeWithId:self.sourceNodeId];
  self.sourceNodeId = nil;
}

- (void)suspend
{
  self.isRunning = false;
}

- (void)resume
{
  self.isRunning = true;
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
  if (!self.isRunning || outputData->mNumberBuffers < 2) {
    return noErr;
  }

  self.renderAudio(outputData, frameCount);

  return noErr;
}

@end

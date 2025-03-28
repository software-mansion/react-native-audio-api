#import <audioapi/ios/core/AudioPlayer.h>
#import <audioapi/ios/system/AudioEngine.h>

@implementation AudioPlayer

- (instancetype)initWithRenderAudio:(RenderAudioBlock)renderAudio
                         sampleRate:(float)sampleRate
                       channelCount:(int)channelCount
{
  if (self = [super init]) {
    self.sampleRate = sampleRate;

    self.channelCount = channelCount;
    self.renderAudio = [renderAudio copy];

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
  NSLog(@"[AudioPlayer] start");
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  self.isRunning = true;
  self.sourceNodeId = [audioEngine attachSourceNode:self.sourceNode format:self.format];
}

- (void)stop
{
  NSLog(@"[AudioPlayer] stop");
  if (!self.isRunning) {
    return;
  }

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  self.isRunning = false;
  [audioEngine detachSourceNodeWithId:self.sourceNodeId];
  self.sourceNodeId = nil;
}

- (void)suspend
{
  NSLog(@"[AudioPlayer] suspend");
  if (!self.isRunning) {
    return;
  }

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  self.isRunning = false;
  [audioEngine detachSourceNodeWithId:self.sourceNodeId];
  self.sourceNodeId = nil;
}

- (void)resume
{
  NSLog(@"[AudioPlayer] resume");
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  self.isRunning = true;
  self.sourceNodeId = [audioEngine attachSourceNode:self.sourceNode format:self.format];
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
  if (outputData->mNumberBuffers < self.channelCount) {
    return noErr; // Ensure we have stereo output
  }

  if (self.isRunning) {
    self.renderAudio(outputData, frameCount);
  }

  return noErr;
}

@end

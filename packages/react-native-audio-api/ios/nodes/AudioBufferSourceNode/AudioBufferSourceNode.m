#import "AudioBufferSourceNode.h"
#import "AudioContext.h"

@implementation AudioBufferSourceNode

- (instancetype)initWithContext:(AudioContext *)context
{
  self = [super initWithContext:context];
  if (self) {
    _loop = true;
    _bufferIndex = 0;
    self.channelCount = 2;
    _buffer = [[RNAA_AudioBuffer alloc] initWithSampleRate:context.sampleRate
                                                    length:[Constants bufferSize]
                                          numberOfChannels:2];

    self.numberOfOutputs = 1;
    self.numberOfInputs = 0;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:_buffer.sampleRate channels:2];
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
    [self.context.audioEngine attachNode:self.sourceNode];
    [self.context.audioEngine connect:self.sourceNode to:self.context.audioEngine.mainMixerNode format:self.format];

    if (!self.context.audioEngine.isRunning) {
      NSError *error = nil;
      if (![self.context.audioEngine startAndReturnError:&error]) {
        NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
      }
    }
  }
  return self;
}

- (void)cleanup
{
  if (self.isPlaying) {
    [self stopPlayback];
  }

  [self.context.audioEngine detachNode:self.sourceNode];
  _format = nil;
}

- (bool)getLoop
{
  return _loop;
}

- (void)setLoop:(bool)loop
{
  _loop = loop;
}

- (void)setBuffer:(RNAA_AudioBuffer *)buffer
{
  _buffer = buffer;
  _mixedBuffer = [_buffer mixWithOutputNumberOfChannels:self.channelCount];
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

  float *leftBufferData = [_mixedBuffer getChannelDataForChannel:0];
  float *rightBufferData = [_mixedBuffer getChannelDataForChannel:1];

  for (int frame = 0; frame < frameCount; frame += 1) {
    // Convert cents to HZ
    if (!self.isPlaying) {
      leftBuffer[frame] = 0;
      rightBuffer[frame] = 0;
      continue;
    }

    leftBuffer[frame] = leftBufferData[_bufferIndex];
    rightBuffer[frame] = rightBufferData[_bufferIndex];

    ++_bufferIndex;

    if (_bufferIndex >= _buffer.length) {
      if (_loop) {
        _bufferIndex = 0;
      } else {
        [self stopPlayback];
        _bufferIndex = 0;
        break;
      }
    }
  }

  [self process:frameCount bufferList:outputData];

  return noErr;
}

@end

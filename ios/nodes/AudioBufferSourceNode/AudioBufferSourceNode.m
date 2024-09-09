#import <AudioBufferSourceNode.h>
#import "Constants.h"

@implementation AudioBufferSourceNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self != [super initWithContext:context]) {
    return self;
  }

    _loop = true;
  _isPlaying = NO;
    _bufferIndex = 0;
    self.channelCount = 2;
    _buffer = [[RNAA_AudioBuffer alloc] initWithSampleRate:context.sampleRate length:[Constants bufferSize] numberOfChannels:2];

  self.numberOfOutputs = 1;
  self.numberOfInputs = 0;

    [self initAudioSourceNode:_buffer];

  return self;
}

- (void)initAudioSourceNode:(RNAA_AudioBuffer *)buffer {
    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:_buffer.sampleRate channels:_buffer.numberOfChannels];
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

- (void)cleanup
{
  if (_isPlaying) {
    [self stopPlayback];
  }

  [self.context.audioEngine detachNode:self.sourceNode];

  _format = nil;
}

- (bool)getLoop {
    return _loop;
}

- (void)setLoop:(bool)loop {
    _loop = loop;
}

- (void)setBuffer:(RNAA_AudioBuffer *)buffer {
    _buffer = buffer;
    [self initAudioSourceNode:_buffer];
}

- (void)start:(double)time
{
  if (_isPlaying) {
    return;
  }

  double delay = time - [self.context getCurrentTime];
  if (delay <= 0) {
    [self startPlayback];
  } else {
    [NSTimer scheduledTimerWithTimeInterval:delay
                                     target:self
                                   selector:@selector(startPlayback)
                                   userInfo:nil
                                    repeats:NO];
  }
}

- (void)startPlayback
{
  _isPlaying = YES;
}

- (void)stop:(double)time
{
  if (!_isPlaying) {
    return;
  }

  double delay = time - [self.context getCurrentTime];
  if (delay <= 0) {
    [self stopPlayback];
  } else {
    [NSTimer scheduledTimerWithTimeInterval:delay target:self selector:@selector(stopPlayback) userInfo:nil repeats:NO];
  }
}

- (void)stopPlayback
{
  _isPlaying = NO;
}

- (OSStatus)renderCallbackWithIsSilence:(BOOL *)isSilence
                              timestamp:(const AudioTimeStamp *)timestamp
                             frameCount:(AVAudioFrameCount)frameCount
                             outputData:(AudioBufferList *)outputData
{
  for (int frame = 0; frame < frameCount; frame += 1) {
    if (!_isPlaying) {
        for (int channel = 0; channel < _buffer.numberOfChannels; channel += 1) {
            outputData->mBuffers[channel].mData = 0;
        }
      continue;
    }
      
      for (int channel = 0; channel < _buffer.numberOfChannels; channel += 1) {
          outputData->mBuffers[channel].mData = [_buffer getChannelDataForChannel:channel][_bufferIndex];
      }
      
      _bufferIndex += 1;

      if (_bufferIndex >= _buffer.length) {
          _bufferIndex = 0;
      }
  }

  [self process:frameCount bufferList:outputData];

  return noErr;
}

- (Boolean)getIsPlaying
{
  return _isPlaying;
}

@end


#import <OscillatorNode.h>
#import "AudioContext.h"
#import "Constants.h"

@implementation OscillatorNode

- (instancetype)initWithContext:(AudioContext *)context
{
  if (self = [super initWithContext:context]) {
    _frequencyParam = [[AudioParam alloc] initWithContext:context
                                                    value:440
                                                 minValue:-[Constants nyquistFrequency]
                                                 maxValue:[Constants nyquistFrequency]];
    _detuneParam = [[AudioParam alloc] initWithContext:context
                                                 value:0
                                              minValue:-[Constants maxDetune]
                                              maxValue:[Constants maxDetune]];
    _waveType = WaveTypeSine;

    self.numberOfOutputs = 1;
    self.numberOfInputs = 0;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.context.sampleRate channels:2];
    _phase = 0.0;

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

  _frequencyParam = nil;
  _detuneParam = nil;
  _format = nil;
  _sourceNode = nil;
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

  float time = [self.context getCurrentTime];
  float deltaTime = 1 / self.context.sampleRate;

  for (int frame = 0; frame < frameCount; frame += 1) {
    // Convert cents to HZ
    if (!self.isPlaying) {
      leftBuffer[frame] = 0;
      rightBuffer[frame] = 0;
      continue;
    }

    double detuneRatio = pow(2.0, [_detuneParam getValueAtTime:time] / OCTAVE_IN_CENTS);
    double detunedFrequency = round(detuneRatio * [_frequencyParam getValueAtTime:time]);
    double phaseIncrement = FULL_CIRCLE_RADIANS * (detunedFrequency / self.context.sampleRate);
    float value = [WaveType valueForWaveType:_waveType atPhase:_phase];
    leftBuffer[frame] = value;
    rightBuffer[frame] = value;

    _phase += phaseIncrement;
    time += deltaTime;

    if (_phase > FULL_CIRCLE_RADIANS) {
      _phase -= FULL_CIRCLE_RADIANS;
    }

    if (_phase < 0) {
      _phase += FULL_CIRCLE_RADIANS;
    }
  }

  if (self.isPlaying) {
    [self process:frameCount bufferList:outputData];
  }

  return noErr;
}

- (void)setType:(NSString *)type
{
  _waveType = [WaveType waveTypeFromString:type];
}

- (NSString *)getType
{
  return [WaveType stringFromWaveType:self.waveType];
}

@end

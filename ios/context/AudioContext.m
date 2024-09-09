#import <AudioContext.h>
#import "Constants.h"

@implementation AudioContext

- (instancetype)init
{
    if (self = [super init]) {
        
        self.audioEngine = [[AVAudioEngine alloc] init];
        self.audioEngine.mainMixerNode.outputVolume = 1;
        self.destination = [[AudioDestinationNode alloc] initWithContext:self];
        
        _contextStartTime = mach_absolute_time();
        _state = ContextStateRunning;
        _sampleRate = [Constants sampleRate];
    }

  return self;
}

- (void)close
{
  [self cleanup];
  _state = ContextStateClosed;
}

- (void)cleanup
{
  if (self.audioEngine.isRunning) {
    [self.audioEngine stop];
  }

  self.audioEngine = nil;
}

- (OscillatorNode *)createOscillator
{
  return [[OscillatorNode alloc] initWithContext:self];
}

- (GainNode *)createGain
{
  return [[GainNode alloc] initWithContext:self];
}

- (StereoPannerNode *)createStereoPanner
{
  return [[StereoPannerNode alloc] initWithContext:self];
}

- (BiquadFilterNode *)createBiquadFilter
{
  return [[BiquadFilterNode alloc] initWithContext:self];
}

- (AudioBufferSourceNode *)createBufferSource
{
  return [[AudioBufferSourceNode alloc] initWithContext:self];
}

- (RNAA_AudioBuffer *)createBuffer:(int)sampleRate length:(int)length numberOfChannels:(int)numberOfChannels {
    return [[RNAA_AudioBuffer alloc] initWithSampleRate:sampleRate length:length numberOfChannels:numberOfChannels];
}

- (double)getCurrentTime
{
  uint64_t currentTime = mach_absolute_time();
  mach_timebase_info_data_t info;
  mach_timebase_info(&info);

  uint64_t elapsedNano = (currentTime - _contextStartTime) * info.numer / info.denom;
  return elapsedNano / 1e9;
}

- (NSString *)getState
{
  return [ContextState toString:_state];
}

- (double)getSampleRate
{
  return _sampleRate;
}

@end

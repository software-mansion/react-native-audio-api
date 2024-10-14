#import <AudioContext.h>
#import "Constants.h"

@implementation AudioContext

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

    self.contextStartTime = mach_absolute_time();
    self.state = ContextStateRunning;
    self.sampleRate = [self.audioSession sampleRate];
  }

  return self;
}

- (void)close
{
  [self cleanup];
  self.state = ContextStateClosed;
}

- (void)cleanup
{
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

- (RNAA_AudioBuffer *)createBuffer:(int)numberOfChannels length:(int)length sampleRate:(int)sampleRate
{
  return [[RNAA_AudioBuffer alloc] initWithNumberOfChannels:numberOfChannels length:length sampleRate:sampleRate];
}

- (double)getCurrentTime
{
  uint64_t currentTime = mach_absolute_time();
  mach_timebase_info_data_t info;
  mach_timebase_info(&info);

  uint64_t elapsedNano = (currentTime - self.contextStartTime) * info.numer / info.denom;
  return elapsedNano / 1e9;
}

- (NSString *)getState
{
  return [ContextState toString:self.state];
}

- (double)getSampleRate
{
  return self.sampleRate;
}

@end

#import <audioapi/ios/core/NativeAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>

@implementation NativeAudioRecorder

static inline uint32_t nextPowerOfTwo(uint32_t x)
{
  if (x == 0) {
    return 1;
  }

  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x++;

  return x;
}

- (AVAudioFormat *)readLiveInputFormat
{
  return [[AudioEngine sharedInstance] getLiveInputFormat];
}

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
{
  if (self = [super init]) {
    self.receiverBlock = [receiverBlock copy];
    self.inputArmed = NO;
    self.resolvedBufferSize = 0;

    __weak typeof(self) weakSelf = self;
    self.receiverSinkBlock = ^OSStatus(
        const AudioTimeStamp *_Nonnull timestamp,
        AVAudioFrameCount frameCount,
        const AudioBufferList *_Nonnull inputData) {
      if (!weakSelf.inputArmed || weakSelf.receiverBlock == nil) {
        return kAudioServicesNoError;
      }

      weakSelf.receiverBlock(inputData, frameCount);

      return kAudioServicesNoError;
    };
  }

  return self;
}

- (AVAudioFormat *)getResolvedInputFormat
{
  return self.resolvedInputFormat;
}

- (int)getBufferSize
{
  AVAudioSession *audioSession = [AVAudioSession sharedInstance];
  double sampleRate = audioSession.sampleRate;

  if (self.resolvedInputFormat != nil && self.resolvedInputFormat.sampleRate > 0) {
    sampleRate = self.resolvedInputFormat.sampleRate;
  }

  float bufferDuration = MAX(audioSession.IOBufferDuration, 0.2);
  return nextPowerOfTwo(ceil(bufferDuration * sampleRate));
}

- (int)getResolvedBufferSize
{
  return self.resolvedBufferSize;
}

- (BOOL)refreshResolvedInputFormatReturningChanged:(BOOL *)formatChanged
{
  AVAudioFormat *liveFormat = [self readLiveInputFormat];

  if (liveFormat == nil || liveFormat.sampleRate <= 0 || liveFormat.channelCount == 0) {
    return NO;
  }

  AVAudioFormat *previousFormat = self.resolvedInputFormat;
  BOOL changed = previousFormat == nil || previousFormat.sampleRate != liveFormat.sampleRate ||
      previousFormat.channelCount != liveFormat.channelCount ||
      previousFormat.isInterleaved != liveFormat.isInterleaved;

  self.resolvedInputFormat = liveFormat;
  self.resolvedBufferSize = [self getBufferSize];

  if (formatChanged != nil) {
    *formatChanged = changed;
  }

  return YES;
}

- (BOOL)start:(NSError **)error
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  // AudioEngine allows us to attach and connect nodes at runtime but with few
  // limitations in this case if it is the first recorder node and player
  // started the engine we need to restart. It can be optimized by tracking if
  // we haven't break rules of at runtime modifications from docs
  // https://developer.apple.com/documentation/avfaudio/avaudioengine?language=objc
  //
  // Currently we are restarting because we do not see any significant performance issue and case when
  // you will need to start and stop recorder very frequently
  self.inputArmed = NO;
  self.resolvedInputFormat = nil;
  self.resolvedBufferSize = 0;

  [audioEngine stopIfNecessary];
  [audioEngine attachInputNodeWithReceiverBlock:self.receiverSinkBlock
                     onInputConfigurationChange:self.onInputConfigurationChange];

  if (![audioEngine startIfNecessary]) {
    [audioEngine detachInputNode];
    [audioEngine stopIfPossible];

    if (error != nil) {
      *error = [NSError
          errorWithDomain:@"NativeAudioRecorder"
                     code:1
                 userInfo:@{
                   NSLocalizedDescriptionKey : @"Failed to start audio engine for recording",
                 }];
    }

    return NO;
  }

  self.resolvedInputFormat = [self readLiveInputFormat];
  self.resolvedBufferSize = [self getBufferSize];

  if (error != nil) {
    *error = nil;
  }

  return YES;
}

- (void)stop
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);
  self.inputArmed = NO;
  [audioEngine detachInputNode];
  [audioEngine stopIfPossible];
  [audioEngine restartAudioEngine];
  self.resolvedInputFormat = nil;
  self.resolvedBufferSize = 0;
}

- (void)pause
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  self.inputArmed = NO;
  [audioEngine pauseIfNecessary];
}

- (void)resume
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  if ([audioEngine startIfNecessary]) {
    if (self.onInputConfigurationChange != nil) {
      self.onInputConfigurationChange();
    } else {
      self.inputArmed = YES;
    }
  }
}

- (void)cleanup
{
  self.inputArmed = NO;
  self.resolvedInputFormat = nil;
  self.resolvedBufferSize = 0;
  self.receiverBlock = nil;
  self.receiverSinkBlock = nil;
  self.onInputConfigurationChange = nil;
}

@end

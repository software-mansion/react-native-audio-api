#import <audioapi/ios/core/NativeAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

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

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
{
  if (self = [super init]) {
    self.receiverBlock = [receiverBlock copy];

    __weak typeof(self) weakSelf = self;
    self.receiverSinkBlock = ^OSStatus(
        const AudioTimeStamp *_Nonnull timestamp,
        AVAudioFrameCount frameCount,
        const AudioBufferList *_Nonnull inputData) {
      weakSelf.receiverBlock(inputData, frameCount);

      return kAudioServicesNoError;
    };

    self.sinkNode = [[AVAudioSinkNode alloc] initWithReceiverBlock:self.receiverSinkBlock];
  }

  return self;
}

// Note: this method should be called only after the session is activated
- (AVAudioFormat *)getInputFormat
{
  AVAudioFormat *format = [AudioEngine.sharedInstance.audioEngine.inputNode inputFormatForBus:0];

  if (format.sampleRate == 0 || format.channelCount == 0) {
    AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];

    format = [[AVAudioFormat alloc]
        initStandardFormatWithSampleRate:[[sessionManager getDevicePreferredSampleRate] doubleValue]
                                channels:[[sessionManager getDevicePreferredInputChannelCount]
                                             intValue]];
  }

  return format;
}

- (int)getBufferSize
{
  // NOTE: this method should be called only after the session is activated
  AVAudioSession *audioSession = [AVAudioSession sharedInstance];

  // TMPfix: it seems that buffer duration in some cases (background/device change) can switch
  // to longer values, exceeding buffer size predicted after session start
  // since it is just a couple of buffers we can set min value of 200ms
  // to enforce we always have enough frames allocated to pass further down the pipeline
  float bufferDuration = MAX(audioSession.IOBufferDuration, 0.2);

  // IOS returns buffer duration rounded, but expects the buffer size to be power of two in runtime
  return nextPowerOfTwo(ceil(bufferDuration * audioSession.sampleRate));
}

- (void)start
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
  [audioEngine stopIfNecessary];
  [audioEngine attachInputNode:self.sinkNode];
  [audioEngine startIfNecessary];
}

- (void)stop
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);
  [audioEngine detachInputNode];
  [audioEngine stopIfPossible];

  // This makes sure that the engine releases the input properly when we no longer need it
  // (i.e. no more misleading dot)
  // Restart only if is not running to avoid interruptions of playback
  if ([audioEngine getState] != AudioEngineStateRunning) {
    [audioEngine restartAudioEngine];
  }
}

- (void)pause
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  [audioEngine pauseIfNecessary];
}

- (void)resume
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  [audioEngine startIfNecessary];
}

- (void)cleanup
{
  self.receiverBlock = nil;
}

@end

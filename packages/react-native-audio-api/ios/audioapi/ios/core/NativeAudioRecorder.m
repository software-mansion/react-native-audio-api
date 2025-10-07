#import <audioapi/ios/core/NativeAudioRecorder.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation NativeAudioRecorder

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
                         bufferLength:(int)bufferLength
                           sampleRate:(float)sampleRate
{
  if (self = [super init]) {
    self.bufferLength = bufferLength;
    self.sampleRate = sampleRate;

    self.receiverBlock = [receiverBlock copy];

    float devicePrefferedSampleRate = [[AVAudioSession sharedInstance] sampleRate];

    if (!devicePrefferedSampleRate) {
      NSError *error;
      devicePrefferedSampleRate = sampleRate;

      [[AVAudioSession sharedInstance] setPreferredSampleRate:sampleRate error:&error];
    }

    self.inputFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                        sampleRate:devicePrefferedSampleRate
                                                          channels:1
                                                       interleaved:NO];
    self.outputFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                         sampleRate:sampleRate
                                                           channels:1
                                                        interleaved:NO];
    self.audioConverter = [[AVAudioConverter alloc] initFromFormat:self.inputFormat toFormat:self.outputFormat];

    __weak typeof(self) weakSelf = self;
    self.receiverSinkBlock = ^OSStatus(
        const AudioTimeStamp *_Nonnull timestamp,
        AVAudioFrameCount frameCount,
        const AudioBufferList *_Nonnull inputData) {
      return [weakSelf processAudioInput:inputData withFrameCount:frameCount atTimestamp:timestamp];
    };

    self.sinkNode = [[AVAudioSinkNode alloc] initWithReceiverBlock:self.receiverSinkBlock];
  }

  return self;
}

- (OSStatus)processAudioInput:(const AudioBufferList *)inputData
               withFrameCount:(AVAudioFrameCount)frameCount
                  atTimestamp:(const AudioTimeStamp *)timestamp
{
  float inputSampleRate = self.inputFormat.sampleRate;
  float outputSampleRate = self.outputFormat.sampleRate;

  if (inputSampleRate != outputSampleRate) {
    AVAudioPCMBuffer *inputBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:self.inputFormat
                                                                  frameCapacity:frameCount];
    memcpy(
        inputBuffer.mutableAudioBufferList->mBuffers[0].mData,
        inputData->mBuffers[0].mData,
        inputData->mBuffers[0].mDataByteSize);
    inputBuffer.frameLength = frameCount;

    int outputFrameCount = frameCount * outputSampleRate / inputSampleRate;

    AVAudioPCMBuffer *outputBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:self.audioConverter.outputFormat
                                                                   frameCapacity:outputFrameCount];

    NSError *error = nil;
    AVAudioConverterInputBlock inputBlock =
        ^AVAudioBuffer *_Nullable(AVAudioPacketCount inNumberOfPackets, AVAudioConverterInputStatus *outStatus)
    {
      *outStatus = AVAudioConverterInputStatus_HaveData;
      return inputBuffer;
    };

    /// IMPORTANT: AVAudioConverter leaks memory without autorelease pool
    /// more details here: https://github.com/poneciak57/AVAudioConverter-memory-leak-repro-electric-boogaloo
    /// we can try to remove it in the future or refactor to reuse buffers to minimize allocations
    @autoreleasepool {
      [self.audioConverter convertToBuffer:outputBuffer error:&error withInputFromBlock:inputBlock];
    }

    if (error) {
      NSLog(@"Error during audio conversion: %@", error.localizedDescription);
      return kAudioServicesBadSpecifierSizeError;
    }

    self.receiverBlock(outputBuffer.audioBufferList, outputBuffer.frameLength);

    return kAudioServicesNoError;
  }

  self.receiverBlock(inputData, frameCount);

  return kAudioServicesNoError;
}

- (void)start
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);

  // AudioEngine allows us to attach and connect nodes at runtime but with few limitations
  // in this case if it is the first recorder node and player started the engine we need to restart.
  // It can be optimized by tracking if we haven't break rules of at runtime modifications from docs
  // https://developer.apple.com/documentation/avfaudio/avaudioengine?language=objc
  //
  // Currently we are restarting because we do not see any significant performance issue and case when
  // you will need to start and stop recorder very frequently
  [audioEngine stopEngine];
  [audioEngine attachInputNode:self.sinkNode];
  [audioEngine startIfNecessary];
}

- (void)stop
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  assert(audioEngine != nil);
  [audioEngine detachInputNode];
  [audioEngine stopIfNecessary];
}

- (void)cleanup
{
  self.receiverBlock = nil;
}

@end

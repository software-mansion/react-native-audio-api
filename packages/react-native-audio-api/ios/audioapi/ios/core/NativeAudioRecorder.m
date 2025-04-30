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

    float devicePrefferedSampleRate = [[[AudioSessionManager sharedInstance] getDevicePreferredSampleRate] floatValue];

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

  AVAudioTime *time = [[AVAudioTime alloc] initWithAudioTimeStamp:timestamp sampleRate:outputSampleRate];

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

    [self.audioConverter convertToBuffer:outputBuffer error:&error withInputFromBlock:inputBlock];

    if (error) {
      NSLog(@"Error during audio conversion: %@", error.localizedDescription);
      return kAudioServicesBadSpecifierSizeError;
    }

    self.receiverBlock(outputBuffer.audioBufferList, outputBuffer.frameLength, time);

    return kAudioServicesNoError;
  }

  self.receiverBlock(inputData, frameCount, time);

  return kAudioServicesNoError;
}

- (void)start
{
  [[AudioEngine sharedInstance] attachInputNode:self.sinkNode];
}

- (void)stop
{
  [[AudioEngine sharedInstance] detachInputNode];
}

- (void)cleanup
{
  self.receiverBlock = nil;
}

@end

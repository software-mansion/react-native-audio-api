#import <AudioDecoder.h>

@implementation AudioDecoder

- (instancetype)initWithSampleRate:(int)sampleRate
{
  if (self = [super init]) {
    self.sampleRate = sampleRate;
  }
  return self;
}

- (const AudioBufferList *)decodeWithFile:(NSString *)path
{
  NSError *error = nil;
  NSURL *fileURL = [NSURL fileURLWithPath:path];
  AVAudioFile *audioFile = [[AVAudioFile alloc] initForReading:fileURL error:&error];

  if (error) {
    NSLog(@"Error occurred while opening the audio file: %@", [error localizedDescription]);
    return nil;
  }
  AVAudioPCMBuffer *buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:[audioFile processingFormat]
                                                           frameCapacity:[audioFile length]];

  AVAudioFormat *format = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                           sampleRate:self.sampleRate
                                                             channels:buffer.audioBufferList->mNumberBuffers
                                                          interleaved:NO];

  [audioFile readIntoBuffer:buffer error:&error];
  if (error) {
    NSLog(@"Error occurred while reading the audio file: %@", [error localizedDescription]);
    return nil;
  }

  if (self.sampleRate != audioFile.processingFormat.sampleRate) {
    self.buffer = [self convertBuffer:buffer ToFormat:format];
  } else {
    self.buffer = buffer;
  }

  return self.buffer.audioBufferList;
}

- (AVAudioPCMBuffer *)convertBuffer:(AVAudioPCMBuffer *)buffer ToFormat:(AVAudioFormat *)format
{
  NSError *error = nil;
  AVAudioConverter *converter = [[AVAudioConverter alloc] initFromFormat:buffer.format toFormat:format];
  AVAudioPCMBuffer *convertedBuffer =
      [[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:(AVAudioFrameCount)buffer.frameCapacity];

  AVAudioConverterInputBlock inputBlock =
      ^AVAudioBuffer *(AVAudioPacketCount inNumberOfPackets, AVAudioConverterInputStatus *outStatus) {
        if (buffer.frameLength > 0) {
          *outStatus = AVAudioConverterInputStatus_HaveData;
          return buffer;
        } else {
          *outStatus = AVAudioConverterInputStatus_NoDataNow;
          return nil;
        }
      };

  [converter convertToBuffer:convertedBuffer error:&error withInputFromBlock:inputBlock];

  if (error) {
    NSLog(@"Error occurred while converting the audio file: %@", [error localizedDescription]);
    return nil;
  }

  return convertedBuffer;
}

- (void)cleanup
{
  self.buffer = nil;
}

@end

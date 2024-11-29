#import <AudioDecoder.h>

@implementation AudioDecoder

- (instancetype)initWithSampleRate:(int)sampleRate
{
  if (self = [super init]) {
    self.sampleRate = sampleRate;
    self.format = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                   sampleRate:self.sampleRate
                                                     channels:2
                                                  interleaved:NO];
  }
  return self;
}

- (const AudioBufferList *)decodeWithFilePath:(NSString *)path
{
  NSError *error = nil;
  NSURL *fileURL = [NSURL fileURLWithPath:path];
  AVAudioFile *audioFile = [[AVAudioFile alloc] initForReading:fileURL error:&error];

  if (error) {
    NSLog(@"Error occurred while opening the audio file: %@", [error localizedDescription]);
    return nil;
  }
  self.buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:[audioFile processingFormat]
                                              frameCapacity:[audioFile length]];

  [audioFile readIntoBuffer:self.buffer error:&error];
  if (error) {
    NSLog(@"Error occurred while reading the audio file: %@", [error localizedDescription]);
    return nil;
  }

  if (self.sampleRate != audioFile.processingFormat.sampleRate) {
    [self convertFromFormat:self.buffer.format];
  }

  return self.buffer.audioBufferList;
}

- (void)convertFromFormat:(AVAudioFormat *)format
{
  NSError *error = nil;
  AVAudioConverter *converter = [[AVAudioConverter alloc] initFromFormat:format toFormat:self.format];
  AVAudioPCMBuffer *convertedBuffer =
      [[AVAudioPCMBuffer alloc] initWithPCMFormat:self.format
                                    frameCapacity:(AVAudioFrameCount)self.buffer.frameCapacity];

  AVAudioConverterInputBlock inputBlock =
      ^AVAudioBuffer *(AVAudioPacketCount inNumberOfPackets, AVAudioConverterInputStatus *outStatus)
  {
    if (self.buffer.frameLength > 0) {
      *outStatus = AVAudioConverterInputStatus_HaveData;
      return self.buffer;
    } else {
      *outStatus = AVAudioConverterInputStatus_NoDataNow;
      return nil;
    }
  };

  [converter convertToBuffer:convertedBuffer error:&error withInputFromBlock:inputBlock];

  if (error) {
    NSLog(@"Error occurred while converting the audio file: %@", [error localizedDescription]);
    return;
  }

  self.buffer = convertedBuffer;
}

- (void)cleanup
{
  self.buffer = nil;
}

@end

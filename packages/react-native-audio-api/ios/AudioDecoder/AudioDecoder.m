#import <AudioDecoder.h>

@implementation AudioDecoder

- (instancetype)initWithSampleRate:(int)sampleRate
{
  if (self = [super init]) {
    self.sampleRate = sampleRate;
  }
  return self;
}

- (const AudioBufferList *)decode:(NSString *)pathOrURL
{
  // check if the input is a URL or a local file path
  NSURL *url = [NSURL URLWithString:pathOrURL];

  if (url && url.scheme) {
    self.buffer = [self decodeWithURL:url];
  } else {
    self.buffer = [self decodeWithFilePath:pathOrURL];
  }

  return self.buffer.audioBufferList;
}

- (AVAudioPCMBuffer *)decodeWithFilePath:(NSString *)path
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
    return [self convertBuffer:buffer ToFormat:format];
  }

  return buffer;
}

- (AVAudioPCMBuffer *)decodeWithURL:(NSURL *)url
{
  __block NSURL *tempFileURL = nil;

  dispatch_group_t group = dispatch_group_create();

  dispatch_group_enter(group);

  [self downloadFileFromURL:url
                 completion:^(NSURL *downloadedFileURL, NSError *downloadError) {
                   if (downloadError) {
                     NSLog(@"Error downloading file: %@", downloadError.localizedDescription);
                     tempFileURL = nil;
                   } else {
                     tempFileURL = downloadedFileURL;
                   }

                   dispatch_group_leave(group);
                 }];

  dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

  if (!tempFileURL) {
    NSLog(@"Cannot process given url");
    return nil;
  }

  return [self decodeWithFilePath:tempFileURL.path];
}

- (void)downloadFileFromURL:(NSURL *)url completion:(void (^)(NSURL *tempFileURL, NSError *error))completion
{
  // get unique file path in temporary dir
  NSString *tempDirectory = NSTemporaryDirectory();
  NSString *timestamp = [NSString stringWithFormat:@"_%@", @((long long)[[NSDate date] timeIntervalSince1970])];
  NSString *fileNameWithTimestamp = [url.lastPathComponent stringByDeletingPathExtension];
  fileNameWithTimestamp = [fileNameWithTimestamp stringByAppendingString:timestamp];
  NSString *fileExtension =
      [url.pathExtension length] > 0 ? [NSString stringWithFormat:@".%@", url.pathExtension] : @"";
  NSString *tempFilePath =
      [tempDirectory stringByAppendingPathComponent:[fileNameWithTimestamp stringByAppendingString:fileExtension]];
  NSURL *tempFileURL = [NSURL fileURLWithPath:tempFilePath];

  // download file
  NSURLSession *session = [NSURLSession sharedSession];
  NSURLSessionDownloadTask *downloadTask = [session
      downloadTaskWithURL:url
        completionHandler:^(NSURL *location, NSURLResponse *response, NSError *error) {
          if (error) {
            NSLog(@"Error downloading file: %@", error.localizedDescription);
            if (completion) {
              completion(nil, error);
            }
            return;
          }

          // move to generated file path in temporary dir
          NSError *fileError = nil;
          BOOL success = [[NSFileManager defaultManager] moveItemAtURL:location toURL:tempFileURL error:&fileError];
          if (success) {
            NSLog(@"File downloaded successfully to %@", tempFileURL.path);
            if (completion) {
              completion(tempFileURL, nil);
            }
          } else {
            NSLog(@"Error moving downloaded file: %@", fileError.localizedDescription);
            if (completion) {
              completion(nil, fileError);
            }
          }
        }];

  [downloadTask resume];
}

- (AVAudioPCMBuffer *)convertBuffer:(AVAudioPCMBuffer *)buffer ToFormat:(AVAudioFormat *)format
{
  NSError *error = nil;
  AVAudioConverter *converter = [[AVAudioConverter alloc] initFromFormat:buffer.format toFormat:format];
  AVAudioPCMBuffer *convertedBuffer =
      [[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:(AVAudioFrameCount)buffer.frameCapacity];

  AVAudioConverterInputBlock inputBlock =
      ^AVAudioBuffer *(AVAudioPacketCount inNumberOfPackets, AVAudioConverterInputStatus *outStatus)
  {
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

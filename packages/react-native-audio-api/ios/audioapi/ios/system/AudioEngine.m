#import <audioapi/ios/system/AudioEngine.h>

@implementation AudioEngine

static AVAudioEngine *audioEngine_;
static NSMutableDictionary *sourceNodes_;
static NSMutableDictionary *sourceFormats_;

- (instancetype)init
{
  if (self == [super init]) {
    audioEngine_ = [[AVAudioEngine alloc] init];
    audioEngine_.mainMixerNode.outputVolume = 1;
    sourceNodes_ = [[NSMutableDictionary alloc] init];
    sourceFormats_ = [[NSMutableDictionary alloc] init];
  }

  return self;
}

+ (void)cleanup
{
  if ([audioEngine_ isRunning]) {
    [audioEngine_ stop];
  }

  audioEngine_ = nil;
  sourceNodes_ = nil;
  sourceFormats_ = nil;
}

+ (bool)rebuildAudioEngine
{
  NSError *error = nil;

  if ([audioEngine_ isRunning]) {
    return true;
  }

  for (id sourceNodeId in sourceNodes_) {
    AVAudioSourceNode *sourceNode = [sourceNodes_ valueForKey:sourceNodeId];
    AVAudioFormat *format = [sourceFormats_ valueForKey:sourceNodeId];

    [audioEngine_ attachNode:sourceNode];
    [audioEngine_ connect:sourceNode to:audioEngine_.mainMixerNode format:format];
  }

  if ([audioEngine_ isRunning]) {
    return true;
  }

  if (![audioEngine_ startAndReturnError:&error]) {
    NSLog(@"Error while rebuilding audio engine: %@", [error debugDescription]);
    return false;
  }

  return true;
}

+ (void)startEngine
{
  NSError *error = nil;

  if ([audioEngine_ isRunning]) {
    return;
  }

  [audioEngine_ startAndReturnError:&error];

  if (error != nil) {
    NSLog(@"Error while starting the audio engine: %@", [error debugDescription]);
  }
}

+ (void)stopEngine
{
  NSLog(@"[IOSAudioManager] stopEngine");
  if (![audioEngine_ isRunning]) {
    return;
  }

  [audioEngine_ pause];
}

+ (bool)isRunning
{
  return [audioEngine_ isRunning];
}

+ (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format
{
  NSString *sourceNodeId = [[NSUUID UUID] UUIDString];
  NSLog(@"[IOSAudioManager] attaching new source node with ID: %@", sourceNodeId);

  [sourceNodes_ setValue:sourceNode forKey:sourceNodeId];
  [sourceFormats_ setValue:format forKey:sourceNodeId];

  [audioEngine_ attachNode:sourceNode];
  [audioEngine_ connect:sourceNode to:audioEngine_.mainMixerNode format:format];

  if ([sourceNodes_ count] == 1) {
    [self startEngine];
  }

  return sourceNodeId;
}

+ (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  NSLog(@"[IOSAudioManager] detaching source nde with ID: %@", sourceNodeId);

  AVAudioSourceNode *sourceNode = [sourceNodes_ valueForKey:sourceNodeId];
  [audioEngine_ detachNode:sourceNode];

  [sourceNodes_ removeObjectForKey:sourceNodeId];
  [sourceFormats_ removeObjectForKey:sourceNodeId];

  if ([sourceNodes_ count] == 0) {
    [self stopEngine];
  }
}

@end

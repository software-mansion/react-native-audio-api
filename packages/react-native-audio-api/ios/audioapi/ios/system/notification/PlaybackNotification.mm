#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/notification/NotificationQueueAssertions.h>
#import <audioapi/ios/system/notification/PlaybackNotification.h>

// Must match PlaybackNotification.DEFAULT_SKIP_INTERVAL_SECONDS on Android.
static const NSInteger kDefaultSkipIntervalSeconds = 15;

static const NSInteger kArtworkMinimumSizeInPixels = 512;
static const NSInteger kArtworkMaximumSizeInPixels = 1024;

#pragma mark - Artwork presentation

/** Lock screen artwork tracks the portrait width, i.e. the screen's short edge. */
static NSInteger ArtworkSizeInPixelsForMainScreen()
{
  UIScreen *screen = [UIScreen mainScreen];
  CGFloat shortEdgePoints = MIN(screen.bounds.size.width, screen.bounds.size.height);
  auto pixels = (NSInteger)(shortEdgePoints * screen.scale);
  return MIN(MAX(pixels, kArtworkMinimumSizeInPixels), kArtworkMaximumSizeInPixels);
}

static UIImage *ArtworkImageScaledToFit(UIImage *image, CGSize requestedSize)
{
  CGSize sourceSize = image.size;
  if (requestedSize.width <= 0 || requestedSize.height <= 0 || sourceSize.width <= 0 ||
      sourceSize.height <= 0) {
    return image;
  }

  CGFloat scale =
      MIN(requestedSize.width / sourceSize.width, requestedSize.height / sourceSize.height);
  // Upscaling would cost memory without adding detail.
  if (scale >= 1.0) {
    return image;
  }

  CGSize targetSize = CGSizeMake(round(sourceSize.width * scale), round(sourceSize.height * scale));

  UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat preferredFormat];
  // Keeps the decoded image's scale, so its points remain its pixels.
  format.scale = 1.0;
  format.opaque = NO;

  UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:targetSize
                                                                             format:format];
  return [renderer imageWithActions:^(UIGraphicsImageRendererContext *rendererContext) {
    [image drawInRect:CGRectMake(0, 0, targetSize.width, targetSize.height)];
  }];
}

/** Wraps a decoded image as artwork that honours the size the system asks for. */
static MPMediaItemArtwork *ArtworkForImage(UIImage *image)
{
  // The handler runs on an arbitrary thread, so it captures the image and nothing of the
  // notification's state.
  return [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size
                                         requestHandler:^UIImage *(CGSize requestedSize) {
                                           return ArtworkImageScaledToFit(image, requestedSize);
                                         }];
}

@implementation PlaybackNotification {
  __weak AudioAPIModule *_audioAPIModule;
  ArtworkLoader *_artworkLoader;

  BOOL _isInitialized;
  BOOL _isActive;
  NSInteger _skipInterval;

  // Metadata, published as a whole by -publishNowPlayingInfo.
  NSString *_title;
  NSString *_artist;
  NSString *_album;
  NSTimeInterval _duration;
  NSTimeInterval _elapsedTime;
  double _speed;
  BOOL _isLiveStream;
  BOOL _isPlaying;
  MPMediaItemArtwork *_artwork;

  NSInteger _artworkMaxPixels;

  id<ArtworkRequest> _artworkRequest;

  /** The artwork currently shown or in flight; the key that de-duplicates repeated updates. */
  NSURL *_displayedArtworkURL;

  /**
   * Incremented by every new artwork request and by -hide. A load captures the generation it
   * started with and its result is dropped if that no longer matches; cancellation alone cannot
   * catch a completion already enqueued when its request was superseded.
   */
  uint64_t _artworkGeneration;
}

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule
                         artworkLoader:(ArtworkLoader *)artworkLoader
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  if (self = [super init]) {
    _audioAPIModule = audioAPIModule;
    _artworkLoader = artworkLoader;
    _isInitialized = false;
    _isActive = false;
    _skipInterval = kDefaultSkipIntervalSeconds;
    _speed = 1.0;
    _artworkMaxPixels = ArtworkSizeInPixelsForMainScreen();
  }

  return self;
}

#pragma mark - BaseNotification Protocol

- (BOOL)initializeWithOptions:(NSDictionary *)options
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  if (_isInitialized) {
    return true;
  }

  [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];

  [self enableRemoteCommand:@"play" enabled:true];
  [self enableRemoteCommand:@"pause" enabled:true];
  [self enableRemoteCommand:@"nextTrack" enabled:true];
  [self enableRemoteCommand:@"previousTrack" enabled:true];
  [self enableRemoteCommand:@"skipForward" enabled:true];
  [self enableRemoteCommand:@"skipBackward" enabled:true];
  [self enableRemoteCommand:@"seekTo" enabled:true];

  _isInitialized = true;
  return true;
}

- (BOOL)showWithOptions:(NSDictionary *)options
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  [self updateSkipIntervalFromOptions:options];

  if (![self initializeWithOptions:options]) {
    return false;
  }

  if (options[@"control"] && options[@"enabled"]) {
    NSString *control = options[@"control"];
    BOOL enabled = [options[@"enabled"] boolValue];
    [self enableControl:control enabled:enabled];
    // Continuing lets us update metadata if provided mixed with controls
  }

  _isActive = true;

  [self updateMetadataFromOptions:options];
  [self publishNowPlayingInfo];

  return true;
}

- (BOOL)hide
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  if (!_isActive) {
    return true;
  }

  // Invalidates any load already on its way back to this queue; see _artworkGeneration.
  _artworkGeneration++;
  [_artworkRequest cancel];
  _artworkRequest = nil;
  _displayedArtworkURL = nil;
  _artwork = nil;

  _title = nil;
  _artist = nil;
  _album = nil;
  _duration = 0;
  _elapsedTime = 0;
  _speed = 1.0;
  _isLiveStream = NO;
  _isPlaying = NO;

  MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
  center.nowPlayingInfo = nil;
  // On iOS clearing nowPlayingInfo is what dismisses the entry; see -publishNowPlayingInfo.
#if TARGET_OS_MACCATALYST
  center.playbackState = MPNowPlayingPlaybackStateStopped;
#endif

  _isActive = false;

  return true;
}

- (void)cleanup
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  if (_isActive) {
    [self hide];
  }

  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];
  [remoteCenter.playCommand removeTarget:self];
  [remoteCenter.pauseCommand removeTarget:self];
  [remoteCenter.stopCommand removeTarget:self];
  [remoteCenter.nextTrackCommand removeTarget:self];
  [remoteCenter.previousTrackCommand removeTarget:self];
  [remoteCenter.skipForwardCommand removeTarget:self];
  [remoteCenter.skipBackwardCommand removeTarget:self];
  [remoteCenter.seekForwardCommand removeTarget:self];
  [remoteCenter.seekBackwardCommand removeTarget:self];
  [remoteCenter.changePlaybackPositionCommand removeTarget:self];

  [[UIApplication sharedApplication] endReceivingRemoteControlEvents];

  _isInitialized = false;
}

- (BOOL)isActive
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());
  return _isActive;
}

- (NSString *)getNotificationType
{
  return @"playback";
}

#pragma mark - Metadata

- (void)updateSkipIntervalFromOptions:(NSDictionary *)options
{
  id skipIntervalValue = options[@"skipInterval"];
  if (skipIntervalValue == nil) {
    return;
  }

  _skipInterval = (NSInteger)[skipIntervalValue doubleValue];
  [self applySkipIntervals];
}

/** Applies whichever keys this update carries, leaving every other field as it was. */
- (void)updateMetadataFromOptions:(NSDictionary *)options
{
  if (!options) {
    return;
  }

  if (options[@"title"] != nullptr) {
    _title = options[@"title"];
  }
  if (options[@"artist"] != nullptr) {
    _artist = options[@"artist"];
  }
  if (options[@"album"] != nullptr) {
    _album = options[@"album"];
  }
  if (options[@"duration"] != nullptr) {
    _duration = [options[@"duration"] doubleValue];
  }
  if (options[@"elapsedTime"] != nullptr) {
    _elapsedTime = [options[@"elapsedTime"] doubleValue];
  }
  if (options[@"speed"] != nullptr) {
    _speed = [options[@"speed"] doubleValue];
  }
  if (options[@"isLiveStream"] != nullptr) {
    _isLiveStream = [options[@"isLiveStream"] boolValue];
  }
  // Sending isEqualToString: to a non-string from JavaScript would raise, not return NO.
  if ([options[@"state"] isKindOfClass:[NSString class]]) {
    _isPlaying = [options[@"state"] isEqualToString:@"playing"];
  }

  if (options[@"artwork"] != nullptr) {
    NSURL *artworkURL = [self resolveArtworkURL:options[@"artwork"]];
    // An unresolvable value leaves the displayed artwork alone, like an omitted key would.
    if (artworkURL != nil) {
      [self requestArtworkForURL:artworkURL];
    }
  }
}

/**
 * The single writer of MPNowPlayingInfoCenter. The dictionary is rebuilt from this object's
 * fields rather than read back from the center, so a dismissed entry can never be rebuilt out of a
 * surviving key.
 */
- (void)publishNowPlayingInfo
{
  if (!_isActive) {
    return;
  }

  NSMutableDictionary<NSString *, id> *info = [[NSMutableDictionary alloc] init];
  if (_title != nil) {
    info[MPMediaItemPropertyTitle] = _title;
  }
  if (_artist != nil) {
    info[MPMediaItemPropertyArtist] = _artist;
  }
  if (_album != nil) {
    info[MPMediaItemPropertyAlbumTitle] = _album;
  }
  if (_artwork != nil) {
    info[MPMediaItemPropertyArtwork] = _artwork;
  }
  info[MPMediaItemPropertyPlaybackDuration] = @(_duration);
  info[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(_elapsedTime);
  // The system advances its scrubber by this rate, so a paused item must report zero.
  info[MPNowPlayingInfoPropertyPlaybackRate] = @(_isPlaying ? _speed : 0.0);
  info[MPNowPlayingInfoPropertyIsLiveStream] = @(_isLiveStream);

  MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
  center.nowPlayingInfo = info;
  // On iOS the playback state is inferred from the audio session and the rate above; setting it
  // requires a private entitlement and is refused with an "Ignoring setPlaybackState" log.
#if TARGET_OS_MACCATALYST
  center.playbackState =
      _isPlaying ? MPNowPlayingPlaybackStatePlaying : MPNowPlayingPlaybackStatePaused;
#endif
}

#pragma mark - Artwork

/**
 * Resolves the artwork value sent from JavaScript, a string or a map carrying a `uri`, to a URL the
 * loader can fetch, or nil when it names nothing reachable.
 */
- (NSURL *)resolveArtworkURL:(id)source
{
  NSString *value = nil;
  if ([source isKindOfClass:[NSString class]]) {
    value = source;
  } else if ([source isKindOfClass:[NSDictionary class]]) {
    value = ((NSDictionary *)source)[@"uri"];
  }

  if (![value isKindOfClass:[NSString class]] || value.length == 0) {
    return nil;
  }

  if ([value hasPrefix:@"http://"] || [value hasPrefix:@"https://"] ||
      [value hasPrefix:@"file://"]) {
    return [NSURL URLWithString:value];
  }

  if ([value hasPrefix:@"/"]) {
    return [NSURL fileURLWithPath:value];
  }

  // A bare name refers to a resource bundled with the host app.
  NSString *path = [[NSBundle mainBundle] pathForResource:value ofType:nil];
  return path != nil ? [NSURL fileURLWithPath:path] : nil;
}

- (void)requestArtworkForURL:(NSURL *)url
{
  // Covers art still loading as well as art displayed, so a per-second elapsedTime update does not
  // restart the same download on every tick.
  if ([url.absoluteString isEqualToString:_displayedArtworkURL.absoluteString]) {
    return;
  }

  [_artworkRequest cancel];
  _displayedArtworkURL = url;

  uint64_t generation = ++_artworkGeneration;
  // The fetch owns this block: a strong capture would keep a torn-down notification alive until the
  // deadline and let it publish afterwards.
  __weak PlaybackNotification *weakSelf = self;
  _artworkRequest = [_artworkLoader loadArtworkFromURL:url
                                   maximumSizeInPixels:_artworkMaxPixels
                                            completion:^(UIImage *image) {
                                              [weakSelf applyLoadedArtworkImage:image
                                                                  forGeneration:generation];
                                            }];
}

/** Applies artwork that finished loading; a stale @c generation is discarded. */
- (void)applyLoadedArtworkImage:(UIImage *)image forGeneration:(uint64_t)generation
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  if (generation != _artworkGeneration) {
    return;
  }
  _artworkRequest = nil;

  if (image == nil) {
    // Clearing the key lets the same address be retried after a transient failure. Artwork already
    // published is left in place: a failed load is not an instruction to remove art.
    _displayedArtworkURL = nil;
    return;
  }

  _artwork = ArtworkForImage(image);
  [self publishNowPlayingInfo];
}

#pragma mark - Remote Commands

- (void)applySkipIntervals
{
  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];
  remoteCenter.skipForwardCommand.preferredIntervals = @[ @(_skipInterval) ];
  remoteCenter.skipBackwardCommand.preferredIntervals = @[ @(_skipInterval) ];
}

- (void)enableControl:(NSString *)control enabled:(BOOL)enabled
{
  NSSet *validControls = [NSSet setWithObjects:@"play",
                                               @"pause",
                                               @"stop",
                                               @"nextTrack",
                                               @"previousTrack",
                                               @"skipForward",
                                               @"skipBackward",
                                               @"seekTo",
                                               nil];
  if ([validControls containsObject:control]) {
    [self enableRemoteCommand:control enabled:enabled];
  }
}

- (void)enableRemoteCommand:(NSString *)name enabled:(BOOL)enabled
{
  if ([name isEqualToString:@"skipForward"] || [name isEqualToString:@"skipBackward"]) {
    [self applySkipIntervals];
  }

  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];

  if ([name isEqualToString:@"play"]) {
    [self enableCommand:remoteCenter.playCommand withSelector:@selector(onPlay:) enabled:enabled];
  } else if ([name isEqualToString:@"pause"]) {
    [self enableCommand:remoteCenter.pauseCommand withSelector:@selector(onPause:) enabled:enabled];
  } else if ([name isEqualToString:@"stop"]) {
    [self enableCommand:remoteCenter.stopCommand withSelector:@selector(onStop:) enabled:enabled];
  } else if ([name isEqualToString:@"nextTrack"]) {
    [self enableCommand:remoteCenter.nextTrackCommand
           withSelector:@selector(onNextTrack:)
                enabled:enabled];
  } else if ([name isEqualToString:@"previousTrack"]) {
    [self enableCommand:remoteCenter.previousTrackCommand
           withSelector:@selector(onPreviousTrack:)
                enabled:enabled];
  } else if ([name isEqualToString:@"skipForward"]) {
    [self enableCommand:remoteCenter.skipForwardCommand
           withSelector:@selector(onSkipForward:)
                enabled:enabled];
  } else if ([name isEqualToString:@"skipBackward"]) {
    [self enableCommand:remoteCenter.skipBackwardCommand
           withSelector:@selector(onSkipBackward:)
                enabled:enabled];
  } else if ([name isEqualToString:@"seekForward"]) {
    [self enableCommand:remoteCenter.seekForwardCommand
           withSelector:@selector(onSeekForward:)
                enabled:enabled];
  } else if ([name isEqualToString:@"seekBackward"]) {
    [self enableCommand:remoteCenter.seekBackwardCommand
           withSelector:@selector(onSeekBackward:)
                enabled:enabled];
  } else if ([name isEqualToString:@"seekTo"]) {
    [self enableCommand:remoteCenter.changePlaybackPositionCommand
           withSelector:@selector(onChangePlaybackPosition:)
                enabled:enabled];
  }
}

- (void)enableCommand:(MPRemoteCommand *)command withSelector:(SEL)selector enabled:(BOOL)enabled
{
  [command removeTarget:self];
  command.enabled = enabled;
  if (enabled) {
    [command addTarget:self action:selector];
  }
}

#pragma mark - Remote Command Handlers

- (MPRemoteCommandHandlerStatus)onPlay:(MPRemoteCommandEvent *)event
{
  [_audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_PLAY
                                      payload:audioapi::EmptyPayload{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onPause:(MPRemoteCommandEvent *)event
{
  [_audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_PAUSE
                                      payload:audioapi::EmptyPayload{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onStop:(MPRemoteCommandEvent *)event
{
  [_audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_STOP
                                      payload:audioapi::EmptyPayload{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onNextTrack:(MPRemoteCommandEvent *)event
{
  [_audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_NEXT_TRACK
                                      payload:audioapi::EmptyPayload{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onPreviousTrack:(MPRemoteCommandEvent *)event
{
  [_audioAPIModule
      invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_PREVIOUS_TRACK
                         payload:audioapi::EmptyPayload{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSeekForward:(MPRemoteCommandEvent *)event
{
  [_audioAPIModule
      invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_SEEK_FORWARD
                         payload:audioapi::EmptyPayload{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSeekBackward:(MPRemoteCommandEvent *)event
{
  [_audioAPIModule
      invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_SEEK_BACKWARD
                         payload:audioapi::EmptyPayload{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSkipForward:(MPSkipIntervalCommandEvent *)event
{
  [_audioAPIModule
      invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_SKIP_FORWARD
                         payload:audioapi::DoubleValuePayload{.value = event.interval}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSkipBackward:(MPSkipIntervalCommandEvent *)event
{
  [_audioAPIModule
      invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_SKIP_BACKWARD
                         payload:audioapi::DoubleValuePayload{.value = event.interval}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onChangePlaybackPosition:
    (MPChangePlaybackPositionCommandEvent *)event
{
  [_audioAPIModule
      invokeHandlerWithEventName:audioapi::AudioEvent::PLAYBACK_NOTIFICATION_SEEK_TO
                         payload:audioapi::DoubleValuePayload{.value = event.positionTime}];
  return MPRemoteCommandHandlerStatusSuccess;
}

@end

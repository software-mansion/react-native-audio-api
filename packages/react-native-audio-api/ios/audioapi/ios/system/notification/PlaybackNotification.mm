#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/notification/PlaybackNotification.h>

#define NOW_PLAYING_INFO_KEYS \
  @{ \
    @"title" : MPMediaItemPropertyTitle, \
    @"artist" : MPMediaItemPropertyArtist, \
    @"album" : MPMediaItemPropertyAlbumTitle, \
    @"duration" : MPMediaItemPropertyPlaybackDuration, \
    @"elapsedTime" : MPNowPlayingInfoPropertyElapsedPlaybackTime, \
    @"speed" : MPNowPlayingInfoPropertyPlaybackRate, \
    @"artwork" : MPMediaItemPropertyArtwork \
  }

@implementation PlaybackNotification {
  BOOL _isInitialized;
  NSMutableDictionary *_currentInfo;
}

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule
{
  if (self = [super init]) {
    self.audioAPIModule = audioAPIModule;
    self.playingInfoCenter = [MPNowPlayingInfoCenter defaultCenter];
    _isInitialized = false;
    _isActive = false;
    _currentInfo = [[NSMutableDictionary alloc] init];
  }

  return self;
}

#pragma mark - BaseNotification Protocol

- (BOOL)initializeWithOptions:(NSDictionary *)options
{
  if (_isInitialized) {
    return true;
  }

  // Enable remote control events
  dispatch_async(dispatch_get_main_queue(), ^{
    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
  });

  // Enable default remote commands
  [self enableRemoteCommand:@"play" enabled:true];
  [self enableRemoteCommand:@"pause" enabled:true];
  [self enableRemoteCommand:@"next" enabled:true];
  [self enableRemoteCommand:@"previous" enabled:true];
  [self enableRemoteCommand:@"skipForward" enabled:true];
  [self enableRemoteCommand:@"skipBackward" enabled:true];
  [self enableRemoteCommand:@"seek" enabled:true];

  _isInitialized = true;
  return true;
}

- (BOOL)showWithOptions:(NSDictionary *)options
{
  if (!_isInitialized) {
    if (![self initializeWithOptions:options]) {
      return false;
    }
  }

  // Handle control enable/disable
  if (options[@"control"] && options[@"enabled"]) {
    NSString *control = options[@"control"];
    BOOL enabled = [options[@"enabled"] boolValue];
    [self enableControl:control enabled:enabled];
    // If it's a control update, we can return early or continue
    // Continuing lets us update metadata if provided mixed with controls
  }

  // Update the now playing info
  [self updateNowPlayingInfo:options];

  _isActive = true;

  return true;
}

- (BOOL)hide
{
  if (!_isActive) {
    return true;
  }

  // Clear now playing info
  self.playingInfoCenter.nowPlayingInfo = nil;
  self.artworkUrl = nil;
  [_currentInfo removeAllObjects];

  _isActive = false;

  return true;
}

- (void)cleanup
{
  // Hide if active
  if (_isActive) {
    [self hide];
  }

  // Disable all remote commands
  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];
  [remoteCenter.playCommand removeTarget:self];
  [remoteCenter.pauseCommand removeTarget:self];
  [remoteCenter.stopCommand removeTarget:self];
  [remoteCenter.togglePlayPauseCommand removeTarget:self];
  [remoteCenter.nextTrackCommand removeTarget:self];
  [remoteCenter.previousTrackCommand removeTarget:self];
  [remoteCenter.skipForwardCommand removeTarget:self];
  [remoteCenter.skipBackwardCommand removeTarget:self];
  [remoteCenter.seekForwardCommand removeTarget:self];
  [remoteCenter.seekBackwardCommand removeTarget:self];
  [remoteCenter.changePlaybackPositionCommand removeTarget:self];

  // Disable remote control events
  dispatch_async(dispatch_get_main_queue(), ^{
    [[UIApplication sharedApplication] endReceivingRemoteControlEvents];
  });

  _isInitialized = false;
}

- (BOOL)isActive
{
  return _isActive;
}

- (NSString *)getNotificationType
{
  return @"playback";
}

#pragma mark - Private Methods

- (void)updateNowPlayingInfo:(NSDictionary *)info
{
  if (!info) {
    return;
  }

  // Get existing now playing info or create new one
  NSMutableDictionary *nowPlayingInfo = [self.playingInfoCenter.nowPlayingInfo mutableCopy];
  if (!nowPlayingInfo) {
    nowPlayingInfo = [[NSMutableDictionary alloc] init];
  }

  // Map keys from our API to MPNowPlayingInfoCenter keys
  NSDictionary *keyMap = NOW_PLAYING_INFO_KEYS;

  // Only update the keys that are provided in this update
  for (NSString *key in info) {
    NSString *mpKey = keyMap[key];
    if (mpKey) {
      // Handle artwork specially - don't set it directly to nowPlayingInfo
      if ([key isEqualToString:@"artwork"]) {
        _currentInfo[key] = info[key];
      } else {
        nowPlayingInfo[mpKey] = info[key];
        _currentInfo[key] = info[key];
      }
    }
  }

  self.playingInfoCenter.nowPlayingInfo = nowPlayingInfo;

  // Handle playback state
  NSString *state = _currentInfo[@"state"];
  MPNowPlayingPlaybackState playbackState = MPNowPlayingPlaybackStatePaused;

  if (state) {
    if ([state isEqualToString:@"playing"]) {
      playbackState = MPNowPlayingPlaybackStatePlaying;
    } else if ([state isEqualToString:@"paused"]) {
      playbackState = MPNowPlayingPlaybackStatePaused;
    } else {
      playbackState = MPNowPlayingPlaybackStatePaused;
    }
  }

  self.playingInfoCenter.playbackState = playbackState;

  // Handle artwork
  NSString *artworkUrl = [self getArtworkUrl:_currentInfo[@"artwork"]];
  [self updateArtworkIfNeeded:artworkUrl];
}

- (NSString *)getArtworkUrl:(id)artwork
{
  if (!artwork) {
    return nil;
  }

  // Handle both string and dictionary formats
  if ([artwork isKindOfClass:[NSString class]]) {
    return artwork;
  } else if ([artwork isKindOfClass:[NSDictionary class]]) {
    return artwork[@"uri"];
  }

  return nil;
}

- (void)updateArtworkIfNeeded:(NSString *)artworkUrl
{
  if (!artworkUrl) {
    return;
  }

  MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
  if ([artworkUrl isEqualToString:self.artworkUrl] &&
      center.nowPlayingInfo[MPMediaItemPropertyArtwork] != nil) {
    return;
  }

  self.artworkUrl = artworkUrl;

  // Load artwork asynchronously
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
    NSURL *url = nil;
    NSData *imageData = nil;
    UIImage *image = nil;

    @try {
      if ([artworkUrl hasPrefix:@"http://"] || [artworkUrl hasPrefix:@"https://"]) {
        // Remote URL
        url = [NSURL URLWithString:artworkUrl];
        imageData = [NSData dataWithContentsOfURL:url];
      } else {
        // Local file - try as resource or file path
        NSString *imagePath = [[NSBundle mainBundle] pathForResource:artworkUrl ofType:nil];
        if (imagePath) {
          imageData = [NSData dataWithContentsOfFile:imagePath];
        } else {
          // Try as absolute path
          imageData = [NSData dataWithContentsOfFile:artworkUrl];
        }
      }

      if (imageData) {
        image = [UIImage imageWithData:imageData];
      }
    } @catch (NSException *exception) {
      // Failed to load artwork
    }

    if (image) {
      MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc]
          initWithBoundsSize:image.size
              requestHandler:^UIImage *_Nonnull(CGSize size) { return image; }];

      dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableDictionary *nowPlayingInfo = [center.nowPlayingInfo mutableCopy];
        if (!nowPlayingInfo) {
          nowPlayingInfo = [[NSMutableDictionary alloc] init];
        }
        nowPlayingInfo[MPMediaItemPropertyArtwork] = artwork;
        center.nowPlayingInfo = nowPlayingInfo;
      });
    }
  });
}

- (void)enableControl:(NSString *)control enabled:(BOOL)enabled
{
  NSSet *validControls = [NSSet setWithObjects:@"play",
                                               @"pause",
                                               @"next",
                                               @"previous",
                                               @"skipForward",
                                               @"skipBackward",
                                               @"seek",
                                               nil];
  if ([validControls containsObject:control]) {
    [self enableRemoteCommand:control enabled:enabled];
  }
}

- (void)enableRemoteCommand:(NSString *)name enabled:(BOOL)enabled
{
  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];

  if ([name isEqualToString:@"play"]) {
    [self enableCommand:remoteCenter.playCommand withSelector:@selector(onPlay:) enabled:enabled];
  } else if ([name isEqualToString:@"pause"]) {
    [self enableCommand:remoteCenter.pauseCommand withSelector:@selector(onPause:) enabled:enabled];
  } else if ([name isEqualToString:@"stop"]) {
    [self enableCommand:remoteCenter.stopCommand withSelector:@selector(onStop:) enabled:enabled];
  } else if ([name isEqualToString:@"togglePlayPause"]) {
    [self enableCommand:remoteCenter.togglePlayPauseCommand
           withSelector:@selector(onTogglePlayPause:)
                enabled:enabled];
  } else if ([name isEqualToString:@"next"]) {
    [self enableCommand:remoteCenter.nextTrackCommand
           withSelector:@selector(onNextTrack:)
                enabled:enabled];
  } else if ([name isEqualToString:@"previous"]) {
    [self enableCommand:remoteCenter.previousTrackCommand
           withSelector:@selector(onPreviousTrack:)
                enabled:enabled];
  } else if ([name isEqualToString:@"skipForward"]) {
    remoteCenter.skipForwardCommand.preferredIntervals = @[ @(15) ];
    [self enableCommand:remoteCenter.skipForwardCommand
           withSelector:@selector(onSkipForward:)
                enabled:enabled];
  } else if ([name isEqualToString:@"skipBackward"]) {
    remoteCenter.skipBackwardCommand.preferredIntervals = @[ @(15) ];
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
  } else if ([name isEqualToString:@"seek"]) {
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
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationPlay" eventBody:@{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onPause:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationPause" eventBody:@{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onStop:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationStop" eventBody:@{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onTogglePlayPause:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationTogglePlayPause"
                                        eventBody:@{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onNextTrack:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationNext" eventBody:@{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onPreviousTrack:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationPrevious" eventBody:@{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSeekForward:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationSeekForward" eventBody:@{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSeekBackward:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationSeekBackward"
                                        eventBody:@{}];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSkipForward:(MPSkipIntervalCommandEvent *)event
{
  NSDictionary *body = @{@"value" : @(event.interval)};
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationSkipForward"
                                        eventBody:body];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSkipBackward:(MPSkipIntervalCommandEvent *)event
{
  NSDictionary *body = @{@"value" : @(event.interval)};
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationSkipBackward"
                                        eventBody:body];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onChangePlaybackPosition:
    (MPChangePlaybackPositionCommandEvent *)event
{
  NSDictionary *body = @{@"value" : @(event.positionTime)};
  [self.audioAPIModule invokeHandlerWithEventName:@"playbackNotificationSeekTo" eventBody:body];
  return MPRemoteCommandHandlerStatusSuccess;
}

@end

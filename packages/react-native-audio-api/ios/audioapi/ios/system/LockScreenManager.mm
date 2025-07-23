#import <MediaPlayer/MediaPlayer.h>
#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/LockScreenManager.h>

#define LOCK_SCREEN_INFO                                          \
  @{                                                              \
    @"album" : MPMediaItemPropertyAlbumTitle,                     \
    @"artist" : MPMediaItemPropertyArtist,                        \
    @"duration" : MPMediaItemPropertyPlaybackDuration,            \
    @"title" : MPMediaItemPropertyTitle,                          \
    @"speed" : MPNowPlayingInfoPropertyPlaybackRate,              \
    @"elapsedTime" : MPNowPlayingInfoPropertyElapsedPlaybackTime, \
  }

@implementation LockScreenManager

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule
{
  if (self = [super init]) {
    self.audioAPIModule = audioAPIModule;
    self.playingInfoCenter = [MPNowPlayingInfoCenter defaultCenter];

    dispatch_async(dispatch_get_main_queue(), ^{
      [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    });
  }

  return self;
}

- (void)cleanup
{
  NSLog(@"[LockScreenManager] cleanup");
  [self resetLockScreenInfo];

  dispatch_async(dispatch_get_main_queue(), ^{
    [[UIApplication sharedApplication] endReceivingRemoteControlEvents];
  });
}

- (void)setLockScreenInfo:(NSDictionary *)info
{
  self.playingInfoCenter = [MPNowPlayingInfoCenter defaultCenter];

  // now playing info(lock screen info)
  NSMutableDictionary *lockScreenInfoDict;

  if (self.playingInfoCenter.nowPlayingInfo == nil) {
    lockScreenInfoDict = [NSMutableDictionary dictionary];
  } else {
    lockScreenInfoDict = [[NSMutableDictionary alloc] initWithDictionary:self.playingInfoCenter.nowPlayingInfo];
  }

  for (NSString *key in LOCK_SCREEN_INFO) {
    if ([info objectForKey:key] != nil) {
      [lockScreenInfoDict setValue:[info objectForKey:key] forKey:[LOCK_SCREEN_INFO objectForKey:key]];
    }
  }

  self.playingInfoCenter.nowPlayingInfo = lockScreenInfoDict;

  // playback state
  NSString *state = [info objectForKey:@"state"];

  if (state != nil) {
    if ([state isEqualToString:@"state_playing"]) {
      self.playingInfoCenter.playbackState = MPNowPlayingPlaybackStatePlaying;
    } else if ([state isEqualToString:@"state_paused"]) {
      self.playingInfoCenter.playbackState = MPNowPlayingPlaybackStatePaused;
    }
  }

  self.playingInfoCenter.playbackState = MPNowPlayingPlaybackStatePaused;

  // artwork
  NSString *artworkUrl = [self getArtworkUrl:[info objectForKey:@"artwork"]];
  [self updateArtworkIfNeeded:artworkUrl];
}

- (void)resetLockScreenInfo
{
  self.playingInfoCenter = [MPNowPlayingInfoCenter defaultCenter];
  self.playingInfoCenter.nowPlayingInfo = nil;
  self.artworkUrl = nil;
}

- (NSString *)getArtworkUrl:(NSString *)artwork
{
  NSString *artworkUrl = nil;

  if (artwork) {
    if ([artwork isKindOfClass:[NSString class]]) {
      artworkUrl = artwork;
    } else if ([[artwork valueForKey:@"uri"] isKindOfClass:[NSString class]]) {
      artworkUrl = [artwork valueForKey:@"uri"];
    }
  }

  return artworkUrl;
}

- (void)updateArtworkIfNeeded:(id)artworkUrl
{
  if (artworkUrl == nil) {
    return;
  }

  MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
  if ([artworkUrl isEqualToString:self.artworkUrl] &&
      [center.nowPlayingInfo objectForKey:MPMediaItemPropertyArtwork] != nil) {
    return;
  }

  self.artworkUrl = artworkUrl;

  // Custom handling of artwork in another thread, will be loaded async
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
    UIImage *image = nil;

    // check whether artwork path is present
    if ([artworkUrl isEqual:@""]) {
      return;
    }

    // artwork is url download from the interwebs
    if ([artworkUrl hasPrefix:@"http://"] || [artworkUrl hasPrefix:@"https://"]) {
      NSURL *imageURL = [NSURL URLWithString:artworkUrl];
      NSData *imageData = [NSData dataWithContentsOfURL:imageURL];
      image = [UIImage imageWithData:imageData];
    } else {
      NSString *localArtworkUrl = [artworkUrl stringByReplacingOccurrencesOfString:@"file://" withString:@""];
      BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:localArtworkUrl];
      if (fileExists) {
        image = [UIImage imageNamed:localArtworkUrl];
      }
    }

    // Check if image was available otherwise don't do anything
    if (image == nil) {
      return;
    }

    // check whether image is loaded
    CGImageRef cgref = [image CGImage];
    CIImage *cim = [image CIImage];

    if (cim == nil && cgref == NULL) {
      return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      // Check if URL wasn't changed in the meantime
      if (![artworkUrl isEqual:self.artworkUrl]) {
        return;
      }

      MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
      MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size
                                                                    requestHandler:^UIImage *_Nonnull(CGSize size) {
                                                                      return image;
                                                                    }];
      NSMutableDictionary *mediaDict = (center.nowPlayingInfo != nil)
          ? [[NSMutableDictionary alloc] initWithDictionary:center.nowPlayingInfo]
          : [NSMutableDictionary dictionary];
      [mediaDict setValue:artwork forKey:MPMediaItemPropertyArtwork];
      center.nowPlayingInfo = mediaDict;
    });
  });
}

- (void)enableRemoteCommand:(NSString *)name enabled:(BOOL)enabled
{
  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];

  if ([name isEqual:@"remotePlay"]) {
    [self enableCommand:remoteCenter.playCommand withSelector:@selector(onPlay:) enabled:enabled];
  } else if ([name isEqual:@"remotePause"]) {
    [self enableCommand:remoteCenter.pauseCommand withSelector:@selector(onPause:) enabled:enabled];
  } else if ([name isEqual:@"remoteStop"]) {
    [self enableCommand:remoteCenter.stopCommand withSelector:@selector(onStop:) enabled:enabled];
  } else if ([name isEqual:@"remoteTogglePlayPause"]) {
    [self enableCommand:remoteCenter.togglePlayPauseCommand withSelector:@selector(onTogglePlayPause:) enabled:enabled];
  } else if ([name isEqual:@"remoteChangePlaybackRate"]) {
    [self enableCommand:remoteCenter.changePlaybackRateCommand
           withSelector:@selector(onChangePlaybackRate:)
                enabled:enabled];
  } else if ([name isEqual:@"remoteNextTrack"]) {
    [self enableCommand:remoteCenter.nextTrackCommand withSelector:@selector(onNextTrack:) enabled:enabled];
  } else if ([name isEqual:@"remotePreviousTrack"]) {
    [self enableCommand:remoteCenter.previousTrackCommand withSelector:@selector(onPreviousTrack:) enabled:enabled];
  } else if ([name isEqual:@"remoteSkipForward"]) {
    remoteCenter.skipForwardCommand.preferredIntervals = @[ @15 ];
    [self enableCommand:remoteCenter.skipForwardCommand withSelector:@selector(onSkipForward:) enabled:enabled];
  } else if ([name isEqual:@"remoteSkipBackward"]) {
    remoteCenter.skipBackwardCommand.preferredIntervals = @[ @15 ];
    [self enableCommand:remoteCenter.skipBackwardCommand withSelector:@selector(onSkipBackward:) enabled:enabled];
  } else if ([name isEqual:@"remoteSeekForward"]) {
    [self enableCommand:remoteCenter.seekForwardCommand withSelector:@selector(onSeekForward:) enabled:enabled];
  } else if ([name isEqual:@"remoteSeekBackward"]) {
    [self enableCommand:remoteCenter.seekBackwardCommand withSelector:@selector(onSeekBackward:) enabled:enabled];
  } else if ([name isEqual:@"remoteChangePlaybackPosition"]) {
    [self enableCommand:remoteCenter.changePlaybackPositionCommand
           withSelector:@selector(onChangePlaybackPosition:)
                enabled:enabled];
  }
}

- (void)enableCommand:(MPRemoteCommand *)command withSelector:(SEL)selector enabled:(BOOL)enabled
{
  [command removeTarget:self action:selector];
  if (enabled) {
    [command addTarget:self action:selector];
  }
  command.enabled = enabled;
}

- (MPRemoteCommandHandlerStatus)onPlay:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"remotePlay" eventBody:nil];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onPause:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"remotePause" eventBody:nil];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onStop:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"remoteStop" eventBody:nil];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onTogglePlayPause:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"remoteTogglePlayPause" eventBody:nil];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onChangePlaybackRate:(MPChangePlaybackRateCommandEvent *)event
{
  NSDictionary *body = @{@"value" : [NSNumber numberWithDouble:event.playbackRate]};

  [self.audioAPIModule invokeHandlerWithEventName:@"remoteChangePlaybackRate" eventBody:body];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onNextTrack:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"remoteNextTrack" eventBody:nil];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onPreviousTrack:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"remotePreviousTrack" eventBody:nil];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSeekForward:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"remoteSeekForward" eventBody:nil];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSeekBackward:(MPRemoteCommandEvent *)event
{
  [self.audioAPIModule invokeHandlerWithEventName:@"remoteSeekBackward" eventBody:nil];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSkipForward:(MPSkipIntervalCommandEvent *)event
{
  NSDictionary *body = @{@"value" : [NSNumber numberWithDouble:event.interval]};

  [self.audioAPIModule invokeHandlerWithEventName:@"remoteSkipForward" eventBody:body];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSkipBackward:(MPSkipIntervalCommandEvent *)event
{
  NSDictionary *body = @{@"value" : [NSNumber numberWithDouble:event.interval]};

  [self.audioAPIModule invokeHandlerWithEventName:@"remoteSkipBackward" eventBody:body];
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onChangePlaybackPosition:(MPChangePlaybackPositionCommandEvent *)event
{
  NSDictionary *body = @{@"value" : [NSNumber numberWithDouble:event.positionTime]};

  [self.audioAPIModule invokeHandlerWithEventName:@"remoteChangePlaybackPosition" eventBody:body];
  return MPRemoteCommandHandlerStatusSuccess;
}

@end

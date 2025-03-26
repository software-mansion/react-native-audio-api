#import <MediaPlayer/MediaPlayer.h>
#import <audioapi/ios/AudioManagerModule.h>
#import <audioapi/ios/system/LockScreenManager.h>

#define LOCK_SCREEN_INFO                                          \
  @{                                                              \
    @"album" : MPMediaItemPropertyAlbumTitle,                     \
    @"artist" : MPMediaItemPropertyArtist,                        \
    @"genre" : MPMediaItemPropertyGenre,                          \
    @"duration" : MPMediaItemPropertyPlaybackDuration,            \
    @"title" : MPMediaItemPropertyTitle,                          \
    @"isLiveStream" : MPNowPlayingInfoPropertyIsLiveStream,       \
    @"speed" : MPNowPlayingInfoPropertyPlaybackRate,              \
    @"elapsedTime" : MPNowPlayingInfoPropertyElapsedPlaybackTime, \
  }

@implementation LockScreenManager

static LockScreenManager *_sharedInstance = nil;

+ (instancetype)sharedInstanceWithAudioManagerModule:(AudioManagerModule *)audioManagerModule
{
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _sharedInstance = [[self alloc] initPrivateWithAudioManagerModule:audioManagerModule];
  });
  return _sharedInstance;
}

- (instancetype)init
{
  @throw [NSException exceptionWithName:@"Singleton" reason:@"Use +[LockScreenManager sharedInstance]" userInfo:nil];
  return nil;
}

- (instancetype)initPrivateWithAudioManagerModule:(AudioManagerModule *)audioManagerModule
{
  if (self = [super init]) {
    self.audioManagerModule = audioManagerModule;
    self.playingInfoCenter = [MPNowPlayingInfoCenter defaultCenter];
    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    [self enableControls];
  }
  return self;
}

- (void)cleanup
{
  NSLog(@"[LockScreenManager] cleanup");
  [self resetLockScreenInfo];
}

- (void)setLockScreenInfo:(NSDictionary *)info
{
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
    } else if ([state isEqualToString:@"state_stopped"]) {
      self.playingInfoCenter.playbackState = MPNowPlayingPlaybackStateStopped;
    }
  }

  // artwork
  NSString *artworkUrl = [self getArtworkUrl:[info objectForKey:@"artwork"]];
  [self updateArtworkIfNeeded:artworkUrl];
}

- (void)resetLockScreenInfo
{
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

- (void)enableControls
{
  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];

  // Playback Commands
  [remoteCenter.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
#ifdef RCT_NEW_ARCH_ENABLED
    [self.audioManagerModule emitOnRemotePlay];
#else
#endif
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  [remoteCenter.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
#ifdef RCT_NEW_ARCH_ENABLED
    [self.audioManagerModule emitOnRemotePause];
#else
#endif
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  [remoteCenter.stopCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
#ifdef RCT_NEW_ARCH_ENABLED
    [self.audioManagerModule emitOnRemoteStop];
#else
#endif
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  [remoteCenter.togglePlayPauseCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
#ifdef RCT_NEW_ARCH_ENABLED
        [self.audioManagerModule emitOnRemoteTogglePlayPause];
#else
#endif
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.changePlaybackRateCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(
                                              MPRemoteCommandEvent *_Nonnull event) {
    MPChangePlaybackRateCommandEvent *changePlaybackRateEvent = (MPChangePlaybackRateCommandEvent *)event;
#ifdef RCT_NEW_ARCH_ENABLED
    [self.audioManagerModule emitOnRemoteChangePlaybackRate:[NSNumber numberWithDouble:changePlaybackRateEvent.playbackRate]];
#else
#endif
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  // Previous/Next Track Commands
  [remoteCenter.nextTrackCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
#ifdef RCT_NEW_ARCH_ENABLED
        [self.audioManagerModule emitOnRemoteNextTrack];
#else
#endif
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.previousTrackCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
#ifdef RCT_NEW_ARCH_ENABLED
        [self.audioManagerModule emitOnRemotePreviousTrack];
#else
#endif
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  // Skip Interval Commands
  [remoteCenter.skipBackwardCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        MPSkipIntervalCommandEvent *skipIntervalEvent = (MPSkipIntervalCommandEvent *)event;
#ifdef RCT_NEW_ARCH_ENABLED
        [self.audioManagerModule emitOnRemoteSkipForward:[NSNumber numberWithDouble:skipIntervalEvent.interval]];
#else
#endif
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.skipForwardCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        MPSkipIntervalCommandEvent *skipIntervalEvent = (MPSkipIntervalCommandEvent *)event;
#ifdef RCT_NEW_ARCH_ENABLED
        [self.audioManagerModule emitOnRemoteSkipForward:[NSNumber numberWithDouble:skipIntervalEvent.interval]];
#else
#endif
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  // Seek Commands
  [remoteCenter.seekForwardCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
#ifdef RCT_NEW_ARCH_ENABLED
        [self.audioManagerModule emitOnRemoteSeekForward];
#else
#endif
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.seekBackwardCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
#ifdef RCT_NEW_ARCH_ENABLED
        [self.audioManagerModule emitOnRemoteSeekBackward];
#else
#endif
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.changePlaybackPositionCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        MPChangePlaybackPositionCommandEvent *changePlaybackPositionEvent =
            (MPChangePlaybackPositionCommandEvent *)event;
#ifdef RCT_NEW_ARCH_ENABLED
        [self.audioManagerModule
            emitOnRemoteChangePlaybackPosition:[NSNumber numberWithDouble:changePlaybackPositionEvent.positionTime]];
#else
#endif
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  remoteCenter.playCommand.enabled = true;
  remoteCenter.pauseCommand.enabled = true;
  remoteCenter.stopCommand.enabled = true;
  remoteCenter.togglePlayPauseCommand.enabled = true;
  remoteCenter.changePlaybackRateCommand.enabled = true;
  remoteCenter.nextTrackCommand.enabled = true;
  remoteCenter.previousTrackCommand.enabled = true;
  remoteCenter.skipBackwardCommand.enabled = true;
  remoteCenter.skipForwardCommand.enabled = true;
  remoteCenter.seekForwardCommand.enabled = true;
  remoteCenter.seekBackwardCommand.enabled = true;
  remoteCenter.changePlaybackPositionCommand.enabled = true;
}

@end

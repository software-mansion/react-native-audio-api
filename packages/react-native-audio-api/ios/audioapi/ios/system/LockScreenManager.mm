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

- (void)enableRemoteCommand:(NSString *)name enabled:(BOOL)enabled
{
  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];

  if ([name isEqual:@"play"]) {
    [self enableCommand:remoteCenter.playCommand withSelector:@selector(onPlay:) enabled:enabled];
  } else if ([name isEqual:@"pause"]) {
    [self enableCommand:remoteCenter.pauseCommand withSelector:@selector(onPause:) enabled:enabled];
  } else if ([name isEqual:@"stop"]) {
    [self enableCommand:remoteCenter.stopCommand withSelector:@selector(onStop:) enabled:enabled];
  } else if ([name isEqual:@"togglePlayPause"]) {
    [self enableCommand:remoteCenter.togglePlayPauseCommand withSelector:@selector(onTogglePlayPause:) enabled:enabled];
  } else if ([name isEqual:@"changePlaybackRate"]) {
    [self enableCommand:remoteCenter.changePlaybackRateCommand
           withSelector:@selector(onChangePlaybackRate:)
                enabled:enabled];
  } else if ([name isEqual:@"nextTrack"]) {
    [self enableCommand:remoteCenter.nextTrackCommand withSelector:@selector(onNextTrack:) enabled:enabled];
  } else if ([name isEqual:@"previousTrack"]) {
    [self enableCommand:remoteCenter.previousTrackCommand withSelector:@selector(onPreviousTrack:) enabled:enabled];
  } else if ([name isEqual:@"skipForward"]) {
    [self enableCommand:remoteCenter.skipForwardCommand withSelector:@selector(onSkipForward:) enabled:enabled];
  } else if ([name isEqual:@"skipBackward"]) {
    [self enableCommand:remoteCenter.skipBackwardCommand withSelector:@selector(onSkipBackward:) enabled:enabled];
  } else if ([name isEqual:@"seekForward"]) {
    [self enableCommand:remoteCenter.seekForwardCommand withSelector:@selector(onSeekForward:) enabled:enabled];
  } else if ([name isEqual:@"seekBackward"]) {
    [self enableCommand:remoteCenter.seekBackwardCommand withSelector:@selector(onSeekBackward:) enabled:enabled];
  } else if ([name isEqual:@"changePlaybackPosition"]) {
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
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemotePlay];
#else
  [self.audioManagerModule sendEventWithName:@"onRemotePlay" body:@{}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onPause:(MPRemoteCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemotePause];
#else
  [self.audioManagerModule sendEventWithName:@"onRemotePause" body:@{}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onStop:(MPRemoteCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteStop];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteStop" body:@{}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onTogglePlayPause:(MPRemoteCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteTogglePlayPause];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteTogglePlayPause" body:@{}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onChangePlaybackRate:(MPChangePlaybackRateCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteChangePlaybackRate:[NSNumber numberWithDouble:event.playbackRate]];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteChangePlaybackRate"
                                        body:@{@"playbackRate" : [NSNumber numberWithDouble:event.playbackRate]}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onNextTrack:(MPRemoteCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteNextTrack];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteNextTrack" body:@{}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onPreviousTrack:(MPRemoteCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemotePreviousTrack];
#else
  [self.audioManagerModule sendEventWithName:@"onRemotePreviousTrack" body:@{}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSeekForward:(MPRemoteCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteSeekForward];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteSeekForward" body:nil];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSeekBackward:(MPRemoteCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteSeekBackward];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteSeekBackward" body:@{}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSkipForward:(MPSkipIntervalCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteSkipForward:[NSNumber numberWithDouble:event.interval]];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteSkipForward"
                                        body:@{@"interval" : [NSNumber numberWithDouble:event.interval]}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onSkipBackward:(MPSkipIntervalCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteSkipBackward:[NSNumber numberWithDouble:event.interval]];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteSkipBackward"
                                        body:@{@"interval" : [NSNumber numberWithDouble:event.interval]}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)onChangePlaybackPosition:(MPChangePlaybackPositionCommandEvent *)event
{
#ifdef RCT_NEW_ARCH_ENABLED
  [self.audioManagerModule emitOnRemoteChangePlaybackPosition:[NSNumber numberWithDouble:event.positionTime]];
#else
  [self.audioManagerModule sendEventWithName:@"onRemoteChangePlaybackPosition"
                                        body:@{@"positionTime" : [NSNumber numberWithDouble:event.positionTime]}];
#endif
  return MPRemoteCommandHandlerStatusSuccess;
}

@end

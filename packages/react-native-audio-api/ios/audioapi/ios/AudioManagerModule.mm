#import <MediaPlayer/MediaPlayer.h>
#import <audioapi/ios/AudioManagerModule.h>

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

@implementation AudioManagerModule

RCT_EXPORT_MODULE(AudioManagerModule);

#ifdef RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeAudioManagerModuleSpecJSI>(params);
}
#endif // RCT_NEW_ARCH_ENABLED

- (void)setLockScreenInfo:(NSDictionary *)info
{
  // lock screen info
  MPNowPlayingInfoCenter *playingInfoCenter = [MPNowPlayingInfoCenter defaultCenter];
  NSMutableDictionary *mediaDict = [NSMutableDictionary dictionary];

  for (NSString *key in LOCK_SCREEN_INFO) {
    if ([info objectForKey:key] != nil) {
      [mediaDict setValue:[info objectForKey:key] forKey:[LOCK_SCREEN_INFO objectForKey:key]];
    }
  }

  playingInfoCenter.nowPlayingInfo = mediaDict;

  // playback state
  NSString *state = [info objectForKey:@"state"];

  if (state != nil) {
    if ([state isEqualToString:@"state_playing"]) {
      playingInfoCenter.playbackState = MPNowPlayingPlaybackStatePlaying;
    } else if ([state isEqualToString:@"state_paused"]) {
      playingInfoCenter.playbackState = MPNowPlayingPlaybackStatePaused;
    } else if ([state isEqualToString:@"state_stopped"]) {
      playingInfoCenter.playbackState = MPNowPlayingPlaybackStateStopped;
    }
  }

  // artwork
  NSString *artworkUrl = [self getArtworkUrl:[info objectForKey:@"artwork"]];
  [self updateArtworkIfNeeded:artworkUrl];
}

- (void)resetLockScreenInfo
{
  MPNowPlayingInfoCenter *playingInfoCenter = [MPNowPlayingInfoCenter defaultCenter];
  playingInfoCenter.nowPlayingInfo = nil;
  self.artworkUrl = nil;
}

- (void)setSessionOptions:(NSString *)category mode:(NSString *)mode options:(NSArray *)options
{
  AVAudioSessionCategory sessionCategory;
  AVAudioSessionMode sessionMode;
  AVAudioSessionCategoryOptions sessionOptions = 0;

  if ([category isEqualToString:@"record"]) {
    sessionCategory = AVAudioSessionCategoryRecord;
  } else if ([category isEqualToString:@"ambient"]) {
    sessionCategory = AVAudioSessionCategoryAmbient;
  } else if ([category isEqualToString:@"playback"]) {
    sessionCategory = AVAudioSessionCategoryPlayback;
  } else if ([category isEqualToString:@"multiRoute"]) {
    sessionCategory = AVAudioSessionCategoryMultiRoute;
  } else if ([category isEqualToString:@"soloAmbient"]) {
    sessionCategory = AVAudioSessionCategorySoloAmbient;
  } else if ([category isEqualToString:@"playAndRecord"]) {
    sessionCategory = AVAudioSessionCategoryPlayAndRecord;
  }

  if ([mode isEqualToString:@"default"]) {
    sessionMode = AVAudioSessionModeDefault;
  } else if ([mode isEqualToString:@"gameChat"]) {
    sessionMode = AVAudioSessionModeGameChat;
  } else if ([mode isEqualToString:@"videoChat"]) {
    sessionMode = AVAudioSessionModeVideoChat;
  } else if ([mode isEqualToString:@"voiceChat"]) {
    sessionMode = AVAudioSessionModeVoiceChat;
  } else if ([mode isEqualToString:@"measurement"]) {
    sessionMode = AVAudioSessionModeMeasurement;
  } else if ([mode isEqualToString:@"voicePrompt"]) {
    sessionMode = AVAudioSessionModeVoicePrompt;
  } else if ([mode isEqualToString:@"spokenAudio"]) {
    sessionMode = AVAudioSessionModeSpokenAudio;
  } else if ([mode isEqualToString:@"moviePlayback"]) {
    sessionMode = AVAudioSessionModeMoviePlayback;
  } else if ([mode isEqualToString:@"videoRecording"]) {
    sessionMode = AVAudioSessionModeVideoRecording;
  }

  for (NSString *option in options) {
    if ([option isEqualToString:@"duckOthers"]) {
      sessionOptions |= AVAudioSessionCategoryOptionDuckOthers;
    }

    if ([option isEqualToString:@"allowAirPlay"]) {
      sessionOptions |= AVAudioSessionCategoryOptionAllowAirPlay;
    }

    if ([option isEqualToString:@"mixWithOthers"]) {
      sessionOptions |= AVAudioSessionCategoryOptionMixWithOthers;
    }

    if ([option isEqualToString:@"allowBluetooth"]) {
      sessionOptions |= AVAudioSessionCategoryOptionAllowBluetooth;
    }

    if ([option isEqualToString:@"defaultToSpeaker"]) {
      sessionOptions |= AVAudioSessionCategoryOptionDefaultToSpeaker;
    }

    if ([option isEqualToString:@"allowBluetoothA2DP"]) {
      sessionOptions |= AVAudioSessionCategoryOptionAllowBluetoothA2DP;
    }

    if ([option isEqualToString:@"overrideMutedMicrophoneInterruption"]) {
      sessionOptions |= AVAudioSessionCategoryOptionOverrideMutedMicrophoneInterruption;
    }

    if ([option isEqualToString:@"interruptSpokenAudioAndMixWithOthers"]) {
      sessionOptions |= AVAudioSessionCategoryOptionInterruptSpokenAudioAndMixWithOthers;
    }
  }

  bool hasDirtySettings = false;

  if (self.sessionCategory != sessionCategory) {
    hasDirtySettings = true;
    self.sessionCategory = sessionCategory;
  }

  if (self.sessionMode != sessionMode) {
    hasDirtySettings = true;
    self.sessionMode = sessionMode;
  }

  if (self.sessionOptions != sessionOptions) {
    hasDirtySettings = true;
    self.sessionOptions = sessionOptions;
  }

  if (hasDirtySettings) {
    [self configureAudioSession];
  }
}

- (NSNumber *)getSampleRate
{
  return [NSNumber numberWithFloat:[self.audioSession sampleRate]];
}

- (id)init
{
  if (self == [super init]) {
    NSLog(@"[IOSAudioManager] init");

    self.audioEngine = [[AudioEngine alloc] init];

    self.sessionCategory = AVAudioSessionCategoryPlayback;
    self.sessionMode = AVAudioSessionModeDefault;
    self.sessionOptions = AVAudioSessionCategoryOptionDuckOthers | AVAudioSessionCategoryOptionAllowBluetooth |
        AVAudioSessionCategoryOptionAllowAirPlay;

    [self configureAudioSession];
    [self configureNotifications];
    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    [self enableControls];
  }

  return self;
}

- (void)cleanup
{
  NSLog(@"[IOSAudioManager] cleanup");
  [AudioEngine cleanup];

  self.audioSession = nil;
  self.notificationCenter = nil;
}

- (void)dealloc
{
  [self cleanup];
}

- (bool)configureAudioSession
{
  NSLog(
      @"[IOSAudioManager] configureAudioSession, category: %@, mode: %@, options: %lu",
      self.sessionCategory,
      self.sessionMode,
      (unsigned long)self.sessionOptions);

  NSError *error = nil;

  if (!self.audioSession) {
    self.audioSession = [AVAudioSession sharedInstance];
  }

  [self.audioSession setCategory:self.sessionCategory mode:self.sessionMode options:self.sessionOptions error:&error];

  if (error != nil) {
    NSLog(@"Error while configuring audio session: %@", [error debugDescription]);
    return false;
  }

  [self.audioSession setActive:true error:&error];

  if (error != nil) {
    NSLog(@"Error while activating audio session: %@", [error debugDescription]);
    return false;
  }

  return true;
}

- (bool)configureNotifications
{
  NSLog(@"[IOSAudioManager] configureNotifications");
  if (!self.notificationCenter) {
    self.notificationCenter = [NSNotificationCenter defaultCenter];
  }

  [self.notificationCenter addObserver:self
                              selector:@selector(handleInterruption:)
                                  name:AVAudioSessionInterruptionNotification
                                object:nil];
  [self.notificationCenter addObserver:self
                              selector:@selector(handleRouteChange:)
                                  name:AVAudioSessionRouteChangeNotification
                                object:nil];
  [self.notificationCenter addObserver:self
                              selector:@selector(handleMediaServicesReset:)
                                  name:AVAudioSessionMediaServicesWereResetNotification
                                object:nil];
  [self.notificationCenter addObserver:self
                              selector:@selector(handleEngineConfigurationChange:)
                                  name:AVAudioEngineConfigurationChangeNotification
                                object:nil];

  return true;
}

- (void)handleInterruption:(NSNotification *)notification
{
  NSError *error;
  UInt8 type = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
  UInt8 option = [[notification.userInfo valueForKey:AVAudioSessionInterruptionOptionKey] intValue];

  if (type == AVAudioSessionInterruptionTypeBegan) {
    self.isInterrupted = true;
    NSLog(@"[IOSAudioManager] Detected interruption, stopping the engine");

    [AudioEngine stopEngine];

    return;
  }

  if (type != AVAudioSessionInterruptionTypeEnded) {
    NSLog(@"[IOSAudioManager] Unexpected interruption state, chilling");
    return;
  }

  self.isInterrupted = false;

  if (option != AVAudioSessionInterruptionOptionShouldResume) {
    NSLog(@"[IOSAudioManager] Interruption ended, but engine is not allowed to resume");
    return;
  }

  NSLog(@"[IOSAudioManager] Interruption ended, resuming the engine");
  bool success = [self.audioSession setActive:true error:&error];

  if (!success) {
    NSLog(@"[IOSAudioManager] Unable to activate the audio session, reason: %@", [error debugDescription]);
    return;
  }

  if (self.hadConfigurationChange) {
    [AudioEngine rebuildAudioEngine];
  }

  [AudioEngine startEngine];
}

- (void)handleRouteChange:(NSNotification *)notification
{
  UInt8 reason = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
  NSLog(@"[IOSAudioManager] Route change detected, reason: %u", reason);

  if (reason == AVAudioSessionRouteChangeReasonOldDeviceUnavailable) {
    NSLog(@"[IOSAudioManager] The previously used audio device became unavailable. Audio engine paused");
    [AudioEngine stopEngine];
  }
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  NSLog(@"[IOSAudioManager] Media services have been reset, tearing down and rebuilding everything.");
  [self cleanup];
  [self configureAudioSession];
  [self configureNotifications];
  [AudioEngine rebuildAudioEngine];
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  if (![AudioEngine isRunning]) {
    NSLog(@"[IOSAudioManager] detected engine configuration change when engine is not running, marking for rebuild");
    self.hadConfigurationChange = true;
    return;
  }

  if (self.isInterrupted) {
    NSLog(@"[IOSAudioManager] detected engine configuration change during interruption, marking for rebuild");
    self.hadConfigurationChange = true;
    return;
  }

  NSLog(@"[IOSAudioManager] detected engine configuration change, rebuilding the graph");
  dispatch_async(dispatch_get_main_queue(), ^{
    [AudioEngine rebuildAudioEngine];
  });
}

- (void)enableControls
{
  MPRemoteCommandCenter *remoteCenter = [MPRemoteCommandCenter sharedCommandCenter];

  // Playback Commands
  [remoteCenter.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
    [self emitOnRemotePlay];
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  [remoteCenter.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
    [self emitOnRemotePause];
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  [remoteCenter.stopCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
    [self emitOnStop];
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  [remoteCenter.togglePlayPauseCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        [self emitOnTogglePlayPause];
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.changePlaybackRateCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        MPChangePlaybackRateCommandEvent *changePlaybackRateEvent = (MPChangePlaybackRateCommandEvent *)event;
        [self emitOnChangePlaybackRate:[NSNumber numberWithDouble:changePlaybackRateEvent.playbackRate]];
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  // Previous/Next Track Commands
  [remoteCenter.nextTrackCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        [self emitOnNextTrack];
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.previousTrackCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        [self emitOnPreviousTrack];
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  // Skip Interval Commands
  [remoteCenter.skipBackwardCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        MPSkipIntervalCommandEvent *skipIntervalEvent = (MPSkipIntervalCommandEvent *)event;
        [self emitOnSkipForward:[NSNumber numberWithDouble:skipIntervalEvent.interval]];
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.skipForwardCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        MPSkipIntervalCommandEvent *skipIntervalEvent = (MPSkipIntervalCommandEvent *)event;
        [self emitOnSkipForward:[NSNumber numberWithDouble:skipIntervalEvent.interval]];
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  // Seek Commands
  [remoteCenter.seekForwardCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        [self emitOnSeekForward];
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.seekBackwardCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        [self emitOnSeekBackward];
        return MPRemoteCommandHandlerStatusSuccess;
      }];

  [remoteCenter.changePlaybackPositionCommand
      addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *_Nonnull event) {
        MPChangePlaybackPositionCommandEvent *changePlaybackPositionEvent =
            (MPChangePlaybackPositionCommandEvent *)event;
        [self emitOnChangePlaybackPosition:[NSNumber numberWithDouble:changePlaybackPositionEvent.positionTime]];
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

@end

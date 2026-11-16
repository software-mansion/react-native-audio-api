#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioAPIModule;

typedef NS_ENUM(NSInteger, InterruptionRecoveryState) {
  /// Nothing is waiting for an interruption to end.
  InterruptionRecoveryStateNone = 0,
  /// An interruption stopped the engine and has not ended yet.
  InterruptionRecoveryStateInterrupted,
  /// The interruption ended, but a recording or a context meant to continue could not restart.
  /// Retried when the app returns to the foreground.
  InterruptionRecoveryStatePending
};

@interface SystemNotificationManager : NSObject

@property (nonatomic, weak) AudioAPIModule *audioAPIModule;
@property (nonatomic, weak) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, strong) NSTimer *hintPollingTimer;
@property (nonatomic, assign) bool hadConfigurationChange;
@property (nonatomic, assign) bool audioInterruptionsObserved;
/// Main queue only.
@property (nonatomic, assign) InterruptionRecoveryState interruptionRecoveryState;
/// `AVAudioSessionInterruptionOptionShouldResume` from the last interruption-ended
/// notification. Nil until that notification arrives; a foreground retry then resumes.
@property (nonatomic, strong) NSNumber *interruptionShouldResume;
@property (nonatomic, assign) bool volumeChangesObserved;
@property (nonatomic, assign) bool wasOtherAudioPlaying;

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule;
- (void)cleanup;

- (void)observeAudioInterruptions:(BOOL)enabled;
- (void)activelyReclaimSession:(BOOL)enabled;
- (void)observeVolumeChanges:(BOOL)enabled;
- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context;

@end

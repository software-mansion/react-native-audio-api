#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioAPIModule;

@interface SystemNotificationManager : NSObject

@property (nonatomic, weak) AudioAPIModule *audioAPIModule;
@property (nonatomic, weak) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, strong) NSTimer *hintPollingTimer;
@property (nonatomic, assign) bool hadConfigurationChange;
@property (nonatomic, assign) bool audioInterruptionsObserved;
/// Main queue. Set when AVAudioSession posts InterruptionEnded (or the secondary-audio
/// equivalent), cleared by the next begin. A foreground retry runs only while it is set.
@property (nonatomic, assign) bool interruptionEndedDelivered;
/// Main queue. `AVAudioSessionInterruptionOptionShouldResume` of the current interruption's
/// ended notification. Nil until it arrives, and again once the engine accepts the next begin,
/// so a later secondary-audio end never replays an earlier interruption's hint; nil resumes.
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

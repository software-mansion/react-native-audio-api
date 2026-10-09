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
@property (nonatomic, assign) bool volumeChangesObserved;
@property (nonatomic, assign) bool wasOtherAudioPlaying;

/// Serial queue on which reactions to system notifications reach the audio engine
/// and session: off the thread that delivered the notification, and in arrival order.
@property (nonatomic, strong, readonly) dispatch_queue_t engineLifecycleQueue;

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

#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioManagerModule;

@interface NotificationManager : NSObject

@property (nonatomic, weak) AudioManagerModule *audioManagerModule;
@property (nonatomic, weak) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, assign) bool hadConfigurationChange;
@property (nonatomic, assign) bool audioInterruptionsObserved;
@property (nonatomic, assign) bool volumeChangesObserved;

- (instancetype)initWithAudioManagerModule:(AudioManagerModule *)audioManagerModule;
- (void)cleanup;

- (void)observeAudioInterruptions:(BOOL)enabled;
- (void)observeVolumeChanges:(BOOL)enabled;
- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context;

@end

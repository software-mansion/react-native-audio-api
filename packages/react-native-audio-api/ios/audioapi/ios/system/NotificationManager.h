#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioAPIModule;

@interface NotificationManager : NSObject

@property (nonatomic, weak) AudioAPIModule *audioAPIModule;
@property (nonatomic, weak) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, assign) bool volumeChangesObserved;
@property (nonatomic, assign) bool audioInterruptionsObserved;

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule;
- (void)cleanup;

- (void)observeAudioInterruptions:(BOOL)enabled;
- (void)observeVolumeChanges:(BOOL)enabled;
- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context;

@end

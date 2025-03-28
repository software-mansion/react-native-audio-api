#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface NotificationManager : NSObject

@property (nonatomic, weak) NSNotificationCenter *notificationCenter;

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, assign) bool hadConfigurationChange;

+ (instancetype)sharedInstance;
- (void)cleanup;

@end

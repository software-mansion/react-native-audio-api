#pragma once

#import <Foundation/Foundation.h>
#import <audioapi/ios/system/notification/BaseNotification.h>

@class AudioAPIModule;

/**
 * NotificationRegistry
 *
 * Central manager for all notification types.
 * Manages registration, lifecycle, and routing of notification implementations.
 */
@interface NotificationRegistry : NSObject

@property (nonatomic, weak) AudioAPIModule *audioAPIModule;

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule;

/**
 * Show a notification. Creates it if it doesn't exist.
 * @param type The notification type identifier
 * @param key The notification key
 * @param options Options for showing the notification
 * @return YES if successful, NO otherwise
 */
- (BOOL)showNotificationWithType:(NSString *)type
                             key:(NSString *)key
                         options:(NSDictionary *)options;

/**
 * Hide a notification.
 * @param key The notification key
 * @return YES if successful, NO otherwise
 */
- (BOOL)hideNotificationWithKey:(NSString *)key;

/**
 * Check if a notification is active.
 * @param key The notification key
 * @return YES if active, NO otherwise
 */
- (BOOL)isNotificationActiveWithKey:(NSString *)key;

/**
 * Clean up all notifications.
 */
- (void)cleanup;

@end

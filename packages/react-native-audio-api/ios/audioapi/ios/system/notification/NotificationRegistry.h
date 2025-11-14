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
 * Register a new notification type.
 * @param type The notification type identifier (e.g., "playback", "recording")
 * @param key Unique key for this notification instance
 * @return YES if registration succeeded, NO otherwise
 */
- (BOOL)registerNotificationType:(NSString *)type withKey:(NSString *)key;

/**
 * Show a registered notification.
 * @param key The notification key
 * @param options Options for showing the notification
 * @return YES if successful, NO otherwise
 */
- (BOOL)showNotificationWithKey:(NSString *)key options:(NSDictionary *)options;

/**
 * Update a shown notification.
 * @param key The notification key
 * @param options Updated options
 * @return YES if successful, NO otherwise
 */
- (BOOL)updateNotificationWithKey:(NSString *)key options:(NSDictionary *)options;

/**
 * Hide a notification.
 * @param key The notification key
 * @return YES if successful, NO otherwise
 */
- (BOOL)hideNotificationWithKey:(NSString *)key;

/**
 * Unregister and clean up a notification.
 * @param key The notification key
 * @return YES if successful, NO otherwise
 */
- (BOOL)unregisterNotificationWithKey:(NSString *)key;

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

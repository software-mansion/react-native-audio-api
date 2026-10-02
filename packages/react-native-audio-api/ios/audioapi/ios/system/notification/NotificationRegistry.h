#pragma once

#import <Foundation/Foundation.h>
#import <audioapi/ios/system/notification/BaseNotification.h>

@class AudioAPIModule;

/**
 * NotificationRegistry
 *
 * Central manager for all notification types.
 * Manages registration, lifecycle, and routing of notification implementations.
 *
 * Every method below hops onto the main queue, which owns the notifications, and answers from there.
 */
@interface NotificationRegistry : NSObject

@property (nonatomic, weak) AudioAPIModule *audioAPIModule;

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule;

/**
 * Show a notification. Creates it if it doesn't exist.
 * @param type The notification type identifier
 * @param key The notification key
 * @param options Options for showing the notification
 * @param completion Receives YES if successful, NO otherwise
 */
- (void)showNotificationWithType:(NSString *)type
                             key:(NSString *)key
                         options:(NSDictionary *)options
                      completion:(void (^)(BOOL success))completion;

/**
 * Hide a notification.
 * @param key The notification key
 * @param completion Receives YES if successful, NO otherwise
 */
- (void)hideNotificationWithKey:(NSString *)key completion:(void (^)(BOOL success))completion;

/**
 * Check if a notification is active.
 * @param key The notification key
 * @param completion Receives YES if active, NO otherwise
 */
- (void)isNotificationActiveWithKey:(NSString *)key completion:(void (^)(BOOL isActive))completion;

/**
 * Clean up all notifications. Blocks until the main queue has run it.
 */
- (void)cleanup;

@end

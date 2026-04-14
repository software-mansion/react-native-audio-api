#pragma once

#import <Foundation/Foundation.h>

/**
 * BaseNotification protocol
 *
 * Interface that all notification types must implement.
 */
@protocol BaseNotification <NSObject>

@required

/**
 * Initialize the notification.
 * @param options Initialization options (can be nil)
 * @return YES if successful
 */
- (BOOL)initializeWithOptions:(NSDictionary *)options;

/**
 * Show the notification (sets metadata on iOS).
 * @param options Notification options
 * @return YES if successful
 */
- (BOOL)showWithOptions:(NSDictionary *)options;

/**
 * Hide the notification (clears metadata on iOS).
 * @return YES if successful
 */
- (BOOL)hide;

/**
 * Clean up and release resources.
 */
- (void)cleanup;

/**
 * Check if notification is active.
 * @return YES if active
 */
- (BOOL)isActive;

/**
 * Get notification type identifier.
 * @return Type identifier (e.g., "playback", "recording")
 */
- (NSString *)getNotificationType;

@end

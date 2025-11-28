#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/notification/NotificationRegistry.h>
#import <audioapi/ios/system/notification/PlaybackNotification.h>

@implementation NotificationRegistry {
  NSMutableDictionary<NSString *, id<BaseNotification>> *_notifications;
}

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule
{
  if (self = [super init]) {
    self.audioAPIModule = audioAPIModule;
    _notifications = [[NSMutableDictionary alloc] init];

    NSLog(@"[NotificationRegistry] Initialized");
  }

  return self;
}

- (BOOL)registerNotificationType:(NSString *)type withKey:(NSString *)key
{
  if (!type || !key) {
    NSLog(@"[NotificationRegistry] Invalid type or key");
    return false;
  }

  // Check if already registered
  if (_notifications[key]) {
    NSLog(@"[NotificationRegistry] Notification with key '%@' already registered", key);
    return false;
  }

  // Create the appropriate notification type
  id<BaseNotification> notification = [self createNotificationForType:type];

  if (!notification) {
    NSLog(@"[NotificationRegistry] Unknown notification type: %@", type);
    return false;
  }

  // Store the notification
  _notifications[key] = notification;

  NSLog(@"[NotificationRegistry] Registered notification type '%@' with key '%@'", type, key);
  return true;
}

- (BOOL)showNotificationWithKey:(NSString *)key options:(NSDictionary *)options
{
  id<BaseNotification> notification = _notifications[key];

  if (!notification) {
    NSLog(@"[NotificationRegistry] No notification found with key: %@", key);
    return false;
  }

  // Initialize if first time showing
  if (![notification isActive]) {
    if (![notification initializeWithOptions:options]) {
      NSLog(@"[NotificationRegistry] Failed to initialize notification: %@", key);
      return false;
    }
  }

  BOOL success = [notification showWithOptions:options];

  if (success) {
    NSLog(@"[NotificationRegistry] Showed notification: %@", key);
  } else {
    NSLog(@"[NotificationRegistry] Failed to show notification: %@", key);
  }

  return success;
}

- (BOOL)updateNotificationWithKey:(NSString *)key options:(NSDictionary *)options
{
  id<BaseNotification> notification = _notifications[key];

  if (!notification) {
    NSLog(@"[NotificationRegistry] No notification found with key: %@", key);
    return false;
  }

  BOOL success = [notification updateWithOptions:options];

  if (success) {
    NSLog(@"[NotificationRegistry] Updated notification: %@", key);
  } else {
    NSLog(@"[NotificationRegistry] Failed to update notification: %@", key);
  }

  return success;
}

- (BOOL)hideNotificationWithKey:(NSString *)key
{
  id<BaseNotification> notification = _notifications[key];

  if (!notification) {
    NSLog(@"[NotificationRegistry] No notification found with key: %@", key);
    return false;
  }

  BOOL success = [notification hide];

  if (success) {
    NSLog(@"[NotificationRegistry] Hid notification: %@", key);
  } else {
    NSLog(@"[NotificationRegistry] Failed to hide notification: %@", key);
  }

  return success;
}

- (BOOL)unregisterNotificationWithKey:(NSString *)key
{
  id<BaseNotification> notification = _notifications[key];

  if (!notification) {
    NSLog(@"[NotificationRegistry] No notification found with key: %@", key);
    return false;
  }

  // Clean up and remove
  [notification cleanup];
  [_notifications removeObjectForKey:key];

  NSLog(@"[NotificationRegistry] Unregistered notification: %@", key);
  return true;
}

- (BOOL)isNotificationActiveWithKey:(NSString *)key
{
  id<BaseNotification> notification = _notifications[key];

  if (!notification) {
    return false;
  }

  return [notification isActive];
}

- (void)cleanup
{
  NSLog(@"[NotificationRegistry] Cleaning up all notifications");

  // Clean up all notifications
  for (id<BaseNotification> notification in [_notifications allValues]) {
    [notification cleanup];
  }

  [_notifications removeAllObjects];
}

#pragma mark - Private Methods

- (id<BaseNotification>)createNotificationForType:(NSString *)type
{
  if ([type isEqualToString:@"playback"]) {
    return [[PlaybackNotification alloc] initWithAudioAPIModule:self.audioAPIModule];
  }
  // Future: Add more notification types here
  // else if ([type isEqualToString:@"recording"]) {
  //   return [[RecordingNotification alloc] initWithAudioAPIModule:self.audioAPIModule];
  // }

  return nil;
}

@end

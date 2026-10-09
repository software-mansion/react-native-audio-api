#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/notification/ArtworkLoader.h>
#import <audioapi/ios/system/notification/NotificationQueueAssertions.h>
#import <audioapi/ios/system/notification/NotificationRegistry.h>
#import <audioapi/ios/system/notification/PlaybackNotification.h>

@implementation NotificationRegistry {
  NSMutableDictionary<NSString *, id<BaseNotification>> *_notifications;
  ArtworkLoader *_artworkLoader;
}

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule
{
  if (self = [super init]) {
    self.audioAPIModule = audioAPIModule;
    _notifications = [[NSMutableDictionary alloc] init];
    _artworkLoader = [[ArtworkLoader alloc] init];
  }

  return self;
}

- (void)showNotificationWithType:(NSString *)type
                             key:(NSString *)key
                         options:(NSDictionary *)options
                      completion:(void (^)(BOOL success))completion
{
  dispatch_async(dispatch_get_main_queue(), ^{
    completion([self showNotificationOnMainQueueWithType:type key:key options:options]);
  });
}

- (void)hideNotificationWithKey:(NSString *)key completion:(void (^)(BOOL success))completion
{
  dispatch_async(
      dispatch_get_main_queue(), ^{ completion([self hideNotificationOnMainQueueWithKey:key]); });
}

- (void)isNotificationActiveWithKey:(NSString *)key completion:(void (^)(BOOL isActive))completion
{
  dispatch_async(dispatch_get_main_queue(), ^{
    id<BaseNotification> notification = self->_notifications[key];
    completion(notification != nil && [notification isActive]);
  });
}

- (void)cleanup
{
  // Synchronous, so teardown cannot race work already queued.
  if ([NSThread isMainThread]) {
    [self cleanupOnMainQueue];
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{ [self cleanupOnMainQueue]; });
  }

  [_artworkLoader cleanup];
}

#pragma mark - Private Methods

- (BOOL)showNotificationOnMainQueueWithType:(NSString *)type
                                        key:(NSString *)key
                                    options:(NSDictionary *)options
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  if (!key) {
    NSLog(@"[NotificationRegistry] Invalid key");
    return false;
  }

  id<BaseNotification> notification = _notifications[key];

  bool created = false;
  if (!notification) {
    if (!type) {
      NSLog(@"[NotificationRegistry] Type required for new notification: %@", key);
      return false;
    }

    notification = [self createNotificationForType:type];

    if (!notification) {
      NSLog(@"[NotificationRegistry] Unknown notification type: %@", type);
      return false;
    }

    _notifications[key] = notification;
    created = true;
  }

  BOOL success = [notification showWithOptions:options];

  if (created && !success) {
    NSLog(@"[NotificationRegistry] Failed to show notification: %@", key);
  }

  return success;
}

- (BOOL)hideNotificationOnMainQueueWithKey:(NSString *)key
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  id<BaseNotification> notification = _notifications[key];

  if (!notification) {
    NSLog(@"[NotificationRegistry] No notification found with key: %@", key);
    return false;
  }

  BOOL success = [notification hide];

  if (!success) {
    NSLog(@"[NotificationRegistry] Failed to hide notification: %@", key);
  }

  return success;
}

- (void)cleanupOnMainQueue
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());

  for (id<BaseNotification> notification in [_notifications allValues]) {
    [notification cleanup];
  }

  [_notifications removeAllObjects];
}

- (id<BaseNotification>)createNotificationForType:(NSString *)type
{
  if ([type isEqualToString:@"playback"]) {
    return [[PlaybackNotification alloc] initWithAudioAPIModule:self.audioAPIModule
                                                  artworkLoader:_artworkLoader];
  }
  // Future: Add more notification types here
  // else if ([type isEqualToString:@"recording"]) {
  //   return [[RecordingNotification alloc] initWithAudioAPIModule:self.audioAPIModule];
  // }

  return nil;
}

@end

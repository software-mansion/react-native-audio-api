#import <ImageIO/ImageIO.h>
#import <audioapi/ios/system/notification/ArtworkLoader.h>
#import <audioapi/ios/system/notification/NotificationQueueAssertions.h>

static const NSTimeInterval kArtworkFetchTimeoutSeconds = 10.0;

/**
 * Ceiling on a downloaded body before it is decoded. Checked once the transfer has completed:
 * aborting mid-stream would need a delegate-based session, and the deadline already bounds the
 * transfer in time.
 */
static const NSUInteger kArtworkMaximumDownloadBytes = 16UL * 1024 * 1024;
static const NSUInteger kArtworkMemoryCacheCapacityBytes = 16UL * 1024 * 1024;
static const NSUInteger kArtworkDiskCacheCapacityBytes = 64UL * 1024 * 1024;

@interface ArtworkLoader ()

@property (nonatomic, readonly) dispatch_queue_t decodeQueue;
@property (nonatomic, readonly) NSURLSession *session;
@property (nonatomic, readonly) NSCache<NSString *, UIImage *> *imageCache;

@end

#pragma mark - Bounded decoding

static NSDictionary *ArtworkThumbnailOptions(NSInteger maximumSizeInPixels)
{
  return @{
    // Embedded thumbnails are typically far too small to display.
    (id)kCGImageSourceCreateThumbnailFromImageAlways : @YES,
    // Bakes the EXIF orientation into the pixels, so the result is upright.
    (id)kCGImageSourceCreateThumbnailWithTransform : @YES,
    // Decodes now, inside the deadline, rather than lazily on whichever thread first draws it.
    (id)kCGImageSourceShouldCacheImmediately : @YES,
    (id)kCGImageSourceThumbnailMaxPixelSize : @(maximumSizeInPixels),
  };
}

/** Decodes @c source at no more than @c maximumSizeInPixels on its longer side, then releases it. */
static UIImage *_Nullable ArtworkImageFromSource(
    CGImageSourceRef _Nullable source,
    NSInteger maximumSizeInPixels)
{
  if (source == NULL) {
    return nil;
  }

  NSDictionary *options = ArtworkThumbnailOptions(maximumSizeInPixels);
  CGImageRef thumbnail =
      CGImageSourceCreateThumbnailAtIndex(source, 0, (__bridge CFDictionaryRef)options);
  CFRelease(source);
  if (thumbnail == NULL) {
    return nil;
  }

  UIImage *image = [UIImage imageWithCGImage:thumbnail scale:1.0 orientation:UIImageOrientationUp];
  CGImageRelease(thumbnail);
  return image;
}

static UIImage *_Nullable ArtworkImageFromData(NSData *data, NSInteger maximumSizeInPixels)
{
  return ArtworkImageFromSource(
      CGImageSourceCreateWithData((__bridge CFDataRef)data, NULL), maximumSizeInPixels);
}

/** Reads through ImageIO rather than NSData, so a large file is never held whole in memory. */
static UIImage *_Nullable ArtworkImageFromFileURL(NSURL *url, NSInteger maximumSizeInPixels)
{
  return ArtworkImageFromSource(
      CGImageSourceCreateWithURL((__bridge CFURLRef)url, NULL), maximumSizeInPixels);
}

#pragma mark - ArtworkFetch

/**
 * A single load in flight. Success, failure, the deadline and cancellation all race to reach
 * @c settleWithImage:notify:, and only the first arrival is delivered.
 *
 * Blocks capture the fetch strongly: the deadline bounds its lifetime, so nothing can leak.
 */
@interface ArtworkFetch : NSObject <ArtworkRequest>

- (instancetype)initWithLoader:(ArtworkLoader *)loader
                           url:(NSURL *)url
           maximumSizeInPixels:(NSInteger)maximumSizeInPixels
                    completion:(ArtworkLoadCompletion)completion;

- (void)start;

@end

@implementation ArtworkFetch {
  ArtworkLoader *_loader;
  NSURL *_url;
  NSString *_cacheKey;
  NSInteger _maximumSizeInPixels;

  // Confined to the main queue.
  ArtworkLoadCompletion _completion;
  NSURLSessionDataTask *_downloadTask;
  dispatch_block_t _timeoutBlock;
  BOOL _hasSettled;
}

- (instancetype)initWithLoader:(ArtworkLoader *)loader
                           url:(NSURL *)url
           maximumSizeInPixels:(NSInteger)maximumSizeInPixels
                    completion:(ArtworkLoadCompletion)completion
{
  if (self = [super init]) {
    _loader = loader;
    _url = url;
    // An image decoded to a different ceiling is not interchangeable, so the ceiling is in the key.
    _cacheKey =
        [NSString stringWithFormat:@"%@|%ld", url.absoluteString, (long)maximumSizeInPixels];
    _maximumSizeInPixels = maximumSizeInPixels;
    _completion = [completion copy];
    _hasSettled = NO;
  }

  return self;
}

- (void)start
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());
  if (_hasSettled) {
    return;
  }

  UIImage *cached = [_loader.imageCache objectForKey:_cacheKey];
  if (cached != nil) {
    [self settleWithImage:cached notify:YES];
    return;
  }

  [self armDeadline];

  if (_url.isFileURL) {
    [self decodeOnDecodeQueue:^{
      return ArtworkImageFromFileURL(self->_url, self->_maximumSizeInPixels);
    }];
    return;
  }

  [self startDownload];
}

- (void)armDeadline
{
  _timeoutBlock = dispatch_block_create(DISPATCH_BLOCK_INHERIT_QOS_CLASS, ^{
    NSLog(
        @"[ArtworkLoader] Artwork fetch timed out after %.0fs: %@",
        kArtworkFetchTimeoutSeconds,
        self->_url);
    [self settleWithImage:nil notify:YES];
  });

  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, (int64_t)(kArtworkFetchTimeoutSeconds * NSEC_PER_SEC)),
      dispatch_get_main_queue(),
      _timeoutBlock);
}

- (void)startDownload
{
  _downloadTask =
      [_loader.session dataTaskWithURL:_url
                     completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                       // Arrives on the session's delegate queue, so hop before touching any confined state.
                       dispatch_async(dispatch_get_main_queue(), ^{
                         [self handleDownloadedData:data response:response error:error];
                       });
                     }];
  [_downloadTask resume];
}

- (void)handleDownloadedData:(NSData *)data
                    response:(NSURLResponse *)response
                       error:(NSError *)error
{
  if (_hasSettled) {
    // A task that was cancelled or that lost to the deadline, reporting in late.
    return;
  }

  if (error != nil) {
    NSLog(
        @"[ArtworkLoader] Failed to download artwork from %@: %@",
        _url,
        error.localizedDescription);
    [self settleWithImage:nil notify:YES];
    return;
  }

  if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
    NSInteger statusCode = ((NSHTTPURLResponse *)response).statusCode;
    if (statusCode < 200 || statusCode > 299) {
      NSLog(@"[ArtworkLoader] Artwork request returned HTTP %ld: %@", (long)statusCode, _url);
      [self settleWithImage:nil notify:YES];
      return;
    }
  }

  if (data.length == 0 || data.length > kArtworkMaximumDownloadBytes) {
    NSLog(
        @"[ArtworkLoader] Rejecting artwork body of %lu bytes: %@",
        (unsigned long)data.length,
        _url);
    [self settleWithImage:nil notify:YES];
    return;
  }

  [self decodeOnDecodeQueue:^{ return ArtworkImageFromData(data, self->_maximumSizeInPixels); }];
}

/**
 * Runs @c decode off the main queue and settles with its result. The deadline can fire
 * meanwhile; ImageIO cannot be interrupted, so the decode finishes and its result is dropped.
 */
- (void)decodeOnDecodeQueue:(UIImage *_Nullable (^)(void))decode
{
  dispatch_async(_loader.decodeQueue, ^{
    UIImage *image = decode();
    if (image == nil) {
      NSLog(@"[ArtworkLoader] Failed to decode artwork from %@", self->_url);
    }

    dispatch_async(dispatch_get_main_queue(), ^{ [self settleWithImage:image notify:YES]; });
  });
}

/** The single exit; only the first arrival is delivered. */
- (void)settleWithImage:(UIImage *_Nullable)image notify:(BOOL)notify
{
  AUDIOAPI_ASSERT_ON_QUEUE(dispatch_get_main_queue());
  if (_hasSettled) {
    return;
  }
  _hasSettled = YES;

  if (_timeoutBlock != nil) {
    dispatch_block_cancel(_timeoutBlock);
    _timeoutBlock = nil;
  }
  [_downloadTask cancel];
  _downloadTask = nil;

  if (image != nil) {
    NSUInteger cost = (NSUInteger)(image.size.width * image.size.height * 4);
    [_loader.imageCache setObject:image forKey:_cacheKey cost:cost];
  }

  ArtworkLoadCompletion completion = _completion;
  // Released now, so a fetch that outlives its caller does not keep the caller's captures alive.
  _completion = nil;

  if (notify && completion != nil) {
    completion(image);
  }
}

- (void)cancel
{
  [self settleWithImage:nil notify:NO];
}

@end

#pragma mark - ArtworkLoader

@implementation ArtworkLoader

- (instancetype)init
{
  if (self = [super init]) {
    // Serial, so concurrent loads cannot each hold a decode buffer at the same time.
    _decodeQueue = dispatch_queue_create(
        "com.swmansion.audioapi.artworkDecode",
        dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_UTILITY, 0));

    NSURLSessionConfiguration *configuration =
        [NSURLSessionConfiguration defaultSessionConfiguration];
    configuration.timeoutIntervalForRequest = kArtworkFetchTimeoutSeconds;
    configuration.timeoutIntervalForResource = kArtworkFetchTimeoutSeconds;
    configuration.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
    // Waiting for connectivity would outlive the deadline.
    configuration.waitsForConnectivity = NO;
    // A private cache, so artwork and the host app's responses cannot evict each other.
    configuration.URLCache =
        [[NSURLCache alloc] initWithMemoryCapacity:kArtworkMemoryCacheCapacityBytes
                                      diskCapacity:kArtworkDiskCacheCapacityBytes
                                          diskPath:@"com.swmansion.audioapi.artwork"];
    _session = [NSURLSession sessionWithConfiguration:configuration];

    _imageCache = [[NSCache alloc] init];
    _imageCache.totalCostLimit = kArtworkMemoryCacheCapacityBytes;
  }

  return self;
}

- (id<ArtworkRequest>)loadArtworkFromURL:(NSURL *)url
                     maximumSizeInPixels:(NSInteger)maximumSizeInPixels
                              completion:(ArtworkLoadCompletion)completion
{
  ArtworkFetch *fetch = [[ArtworkFetch alloc] initWithLoader:self
                                                         url:url
                                         maximumSizeInPixels:maximumSizeInPixels
                                                  completion:completion];

  // Dispatched rather than started inline, so even a cache hit calls back a turn later and callers
  // are never re-entered from inside their own update.
  dispatch_async(dispatch_get_main_queue(), ^{ [fetch start]; });

  return fetch;
}

- (void)cleanup
{
  // Every task in flight fails with NSURLErrorCancelled and settles as a failure on the main queue.
  [_session invalidateAndCancel];
  [_imageCache removeAllObjects];
}

@end

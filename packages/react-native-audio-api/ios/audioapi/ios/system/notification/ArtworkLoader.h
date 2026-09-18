#pragma once

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Abandons a load. Must be called on the main queue.
 */
@protocol ArtworkRequest <NSObject>

- (void)cancel;

@end

/**
 * Receives the outcome of a load: the decoded image, or nil on failure or timeout.
 */
using ArtworkLoadCompletion = void (^)(UIImage *_Nullable image);

/**
 * Fetches and decodes media notification artwork, bounded in size and in time. Every completion is
 * delivered on the main queue, which owns the callers' artwork state.
 *
 * One instance is shared by every notification so that they share its URL session and its caches.
 */
@interface ArtworkLoader : NSObject

/**
 * Fetches @c url, decoded so that its longer side is at most @c maximumSizeInPixels.
 */
- (id<ArtworkRequest>)loadArtworkFromURL:(NSURL *)url
                     maximumSizeInPixels:(NSInteger)maximumSizeInPixels
                              completion:(ArtworkLoadCompletion)completion;

/** Cancels every load in flight and empties the decoded-image cache. */
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END

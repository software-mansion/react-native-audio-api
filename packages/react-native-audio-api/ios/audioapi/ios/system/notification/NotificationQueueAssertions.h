#pragma once

#import <dispatch/dispatch.h>

/**
 * Traps in debug builds when the caller is not on `queue`. Notification state has exactly one owner
 * queue instead of a lock, and this makes that ownership visible at each entry point.
 */
#if defined(DEBUG) && DEBUG
#define AUDIOAPI_ASSERT_ON_QUEUE(queue) dispatch_assert_queue(queue)
#else
#define AUDIOAPI_ASSERT_ON_QUEUE(queue) ((void)0)
#endif

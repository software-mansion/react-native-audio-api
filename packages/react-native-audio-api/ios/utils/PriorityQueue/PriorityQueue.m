#import "PriorityQueue.h"
#import "ParamChange.h"

NSInteger compareValues(const void *val1, const void *val2, void *context)
{
  ParamChange *pc1 = (__bridge ParamChange *)val1;
  ParamChange *pc2 = (__bridge ParamChange *)val2;

  if (pc1.startTime > pc2.startTime)
    return 1;
  if (pc1.startTime < pc2.startTime)
    return -1;

  if (pc1.endTime > pc2.endTime)
    return 1;
  if (pc1.endTime < pc2.endTime)
    return -1;

  return 0;
}

static const void *heapRetain(CFAllocatorRef all, const void *ptr)
{
  return (const void *)CFRetain((CFTypeRef)ptr);
}

static void heapRelease(CFAllocatorRef all, const void *ptr)
{
  CFRelease((CFTypeRef)ptr);
}

@implementation PriorityQueue

- (instancetype)init
{
  self = [super init];
  if (self) {
    CFBinaryHeapCallBacks callbacks;
    callbacks.version = 0;
    callbacks.compare = &compareValues;
    callbacks.retain = &heapRetain;
    callbacks.release = &heapRelease;
    _heap = CFBinaryHeapCreate(kCFAllocatorDefault, 0, &callbacks, NULL);
  }
  return self;
}

- (void)dealloc
{
  if (_heap) {
    CFRelease(_heap);
  }
}

- (void)insert:(id)object
{
  CFBinaryHeapAddValue(_heap, (__bridge const void *)(object));
}

- (id)extractMin
{
  const void *value = CFBinaryHeapGetMinimum(_heap);
  if (value) {
    CFBinaryHeapRemoveMinimumValue(_heap);
    return (__bridge id)value;
  }
  return nil;
}

- (id)peekMin
{
  const void *value = CFBinaryHeapGetMinimum(_heap);
  return (__bridge id)value;
}

- (id)peekMax
{
  const void *value = NULL;
  CFIndex count = CFBinaryHeapGetCount(_heap);
  if (count > 0) {
    const void **values = malloc(count * sizeof(void *));
    CFBinaryHeapGetValues(_heap, values);
    value = values[0];
    for (CFIndex i = 1; i < count; i++) {
      if (compareValues(values[i], value, NULL) > 0) {
        value = values[i];
      }
    }
    free(values);
  }
  return (__bridge id)value;
}

- (NSUInteger)count
{
  return (NSUInteger)CFBinaryHeapGetCount(_heap);
}

@end

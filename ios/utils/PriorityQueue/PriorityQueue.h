#import <Foundation/Foundation.h>

@interface PriorityQueue : NSObject

@property (nonatomic, assign) CFBinaryHeapRef heap;

- (void)insert:(id)object;
- (id)extractMin;
- (id)peekMin;
- (id)peekMax;
- (NSUInteger)count;

@end

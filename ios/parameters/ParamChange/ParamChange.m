#import "ParamChange.h"

@implementation ParamChange

- (instancetype)init:(double)startTime
              endTime:(double)endTime
           startValue:(double)startValue
             endValue:(double)endValue
               target:(id)target
    calculateSelector:(SEL)calculateSelector
{
  if (self != [super init]) {
    return self;
  }

  _startTime = startTime;
  _endTime = endTime;
  _startValue = startValue;
  _endValue = endValue;
  _target = target;
  _calculateSelector = calculateSelector;

  return self;
}

- (double)getValueAtTime:(double)time
{
  if ([self.target respondsToSelector:self.calculateSelector]) {
    NSInvocation *invocation =
        [NSInvocation invocationWithMethodSignature:[self.target methodSignatureForSelector:self.calculateSelector]];
    [invocation setSelector:self.calculateSelector];
    [invocation setTarget:self.target];

    [invocation setArgument:&_startTime atIndex:2];
    [invocation setArgument:&_endTime atIndex:3];
    [invocation setArgument:&_startValue atIndex:4];
    [invocation setArgument:&_endValue atIndex:5];
    [invocation setArgument:&time atIndex:6];

    [invocation invoke];

    double result;
    [invocation getReturnValue:&result];
    return result;
  }

  return 0.0;
}

@end

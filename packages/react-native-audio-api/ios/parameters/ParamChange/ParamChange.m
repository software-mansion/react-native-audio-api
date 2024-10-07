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

  _invocation =
      [NSInvocation invocationWithMethodSignature:[self.target methodSignatureForSelector:self.calculateSelector]];

  [_invocation setSelector:self.calculateSelector];
  [_invocation setTarget:self.target];

  [_invocation setArgument:&_startTime atIndex:2];
  [_invocation setArgument:&_endTime atIndex:3];
  [_invocation setArgument:&_startValue atIndex:4];
  [_invocation setArgument:&_endValue atIndex:5];

  return self;
}

- (double)getValueAtTime:(double)time
{
  if ([self.target respondsToSelector:self.calculateSelector]) {
    [_invocation setArgument:&time atIndex:6];

    [_invocation invoke];

    double result;
    [_invocation getReturnValue:&result];
    return result;
  }

  return 0.0;
}

@end

#import "AudioParam.h"
#import "AudioContext.h"

@implementation AudioParam

- (instancetype)initWithContext:(AudioContext *)context value:(float)value minValue:(float)minValue maxValue:(float)maxValue {
    if (self != [super init]) {
      return self;
    }
    
    _value = value;
    _defaultValue = value;
    _minValue = minValue;
    _maxValue = maxValue;
    _context = context;
    _timeline = [[PriorityQueue alloc] init];

    return self;
}

- (void)clean {
    _timeline = nil;
    _context = nil;
}

- (void)setValue:(float)value {
    if (value < _minValue || value > _maxValue) {
        NSLog(@"Value out of bounds");
        @throw [NSException exceptionWithName:NSInternalInconsistencyException reason:[NSString stringWithFormat:@"Value out of bounds"] userInfo:nil];
    }

    _value = value;
}

- (float)getValue {
    return _value;
}

- (float)getMinValue {
    return _minValue;
}

- (float)getMaxValue {
    return _maxValue;
}

- (float)getDefaultValue {
    return _defaultValue;
}

- (double)getValueAtTime:(double)time {
    if ([_timeline count] <= 0) {
        return _value;
    }

    ParamChange *param = [_timeline peekMin];
    
    if (param.endTime < [self.context getCurrentTime]) {
        [_timeline extractMin];
        _value = param.endValue;
    }
    
    if (time >= param.startTime && time <= param.endTime) {
        _value = [param getValueAtTime:time];
    }
    
    return _value;
}

- (double)getStartTime {
    if ([_timeline count] <= 0) {
        return [self.context getCurrentTime];
    }
    
    ParamChange *param = [_timeline peekMax];
    return param.endTime;
}

- (float)getStartValue {
    if ([_timeline count] <= 0) {
        return _value;
    }
    
    ParamChange *param = [_timeline peekMax];
    return param.endValue;
}

- (void)setValueAtTime:(float)value time:(double)time {
    ParamChange *param = [[ParamChange alloc] init:time endTime:time startValue:value endValue:value target:self calculateSelector:@selector(calculateLinearValueWithStartTime:endTime:startValue:endValue:currentTime:)];
    [_timeline insert:param];
}

- (double)calculateSetValueAtTime:(double)startTime endTime:(double)endTime startValue:(double)startValue endValue:(double)endValue currentTime:(double)currentTime {
    return endValue;
}

- (void)linearRampToValueAtTime:(float)value time:(double)time {
    ParamChange *param = [[ParamChange alloc] init:[self getStartTime] endTime:time startValue:[self getStartValue] endValue:value target:self calculateSelector:@selector(calculateLinearValueWithStartTime:endTime:startValue:endValue:currentTime:)];
    [_timeline insert:param];
}

- (double)calculateLinearValueWithStartTime:(double)startTime endTime:(double)endTime startValue:(double)startValue endValue:(double)endValue currentTime:(double)currentTime {
    return startValue + (endValue - startValue) * (currentTime - startTime) / (endTime - startTime);
}

- (void)exponentialRampToValueAtTime:(float)value time:(double)time {
    ParamChange *param = [[ParamChange alloc] init:[self getStartTime] endTime:time startValue:[self getStartValue] endValue:value target:self calculateSelector:@selector(calculateExponentialRampToValueAtTime:endTime:startValue:endValue:currentTime:)];
    [_timeline insert:param];
}

- (double)calculateExponentialRampToValueAtTime:(double)startTime endTime:(double)endTime startValue:(double)startValue endValue:(double)endValue currentTime:(double)currentTime {
    return startValue * pow((endValue / startValue), (currentTime - startTime) / (endTime - startTime));
}

@end

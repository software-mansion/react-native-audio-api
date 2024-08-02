#import "AudioParam.h"
#import "AudioContext.h"

@implementation AudioParam

- (instancetype)init:(AudioContext *)context value:(float)value minValue:(float)minValue maxValue:(float)maxValue {
    if (self = [super init]) {
        _value = value;
        _defaultValue = value;
        _minValue = minValue;
        _maxValue = maxValue;
        _context = context;
    }

    return self;
}

- (void)setValue:(float)value {
    if (value < _minValue || value > _maxValue) {
        NSLog(@"Value out of bounds");
        @throw [NSException exceptionWithName:NSInternalInconsistencyException reason:[NSString stringWithFormat:@"Value out of bounds"] userInfo:nil];
    }
    
    _value = value;
    [self.context processNodes];
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

@end

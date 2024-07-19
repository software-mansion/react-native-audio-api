#import "WaveType.h"
#import <math.h>

@implementation WaveType

+ (WaveTypeEnum)waveTypeFromString:(NSString *)type {
    if ([type isEqualToString:@"sine"]) {
        return WaveTypeSine;
    } else if ([type isEqualToString:@"square"]) {
        return WaveTypeSquare;
    } else if ([type isEqualToString:@"sawtooth"]) {
        return WaveTypeSawtooth;
    } else if ([type isEqualToString:@"triangle"]) {
        return WaveTypeTriangle;
    } else {
        NSLog(@"Unknown wave type: %@", type);
        return WaveTypeSine; // Default value
    }
}

+ (NSString *)stringFromWaveType:(WaveTypeEnum)waveType {
    switch (waveType) {
        case WaveTypeSine:
            return @"sine";
        case WaveTypeSquare:
            return @"square";
        case WaveTypeSawtooth:
            return @"sawtooth";
        case WaveTypeTriangle:
            return @"triangle";
    }
    return nil;
}

+ (float)valueForWaveType:(WaveTypeEnum)waveType atPhase:(double)phase {
    switch (waveType) {
        case WaveTypeSine:
            return (float)sin(phase);
        case WaveTypeSquare:
            return (float)(sin(phase) >= 0 ? 1 : -1);
        case WaveTypeSawtooth:
            return (float)(2 * (phase / FULL_CIRCLE_RADIANS - floor(phase / FULL_CIRCLE_RADIANS + 0.5)));
        case WaveTypeTriangle:
            return (float)(2 * fabs(2 * (phase / FULL_CIRCLE_RADIANS - floor(phase / FULL_CIRCLE_RADIANS + 0.5))) - 1);
    }
    return 0;
}

@end

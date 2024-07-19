#import "WaveType.h"
#import <math.h>

@implementation WaveType

+ (WaveTypeEnum)waveTypeFromString:(NSString *)type {
    if ([type isEqualToString:@"sine"]) {
        return WaveTypeSine;
    }

    if ([type isEqualToString:@"square"]) {
        return WaveTypeSquare;
    }

    if ([type isEqualToString:@"sawtooth"]) {
        return WaveTypeSawtooth;
    }

    if ([type isEqualToString:@"triangle"]) {
        return WaveTypeTriangle;
    }

    @throw [NSException exceptionWithName:@"Invalid wave type" reason:@"You have to pick correct wave type: ['sine', 'square', 'sawtooth', 'trinagle']" userInfo:nil];
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
        default:
            @throw [NSException exceptionWithName:@"Invalid wave type" reason:@"You have to pick correct wave type: ['sine', 'square', 'sawtooth', 'trinagle']" userInfo:nil];
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
        default:
            @throw [NSException exceptionWithName:@"Invalid wave type" reason:@"You have to pick correct wave type: ['sine', 'square', 'sawtooth', 'trinagle']" userInfo:nil];
    }

    return 0;
}

@end

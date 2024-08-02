#pragma once

static const float FULL_CIRCLE_RADIANS = 2 * M_PI;

typedef NS_ENUM(NSInteger, WaveTypeEnum) {
    WaveTypeSine,
    WaveTypeSquare,
    WaveTypeSawtooth,
    WaveTypeTriangle
};

@interface WaveType : NSObject

+ (WaveTypeEnum)waveTypeFromString:(NSString *)type;
+ (NSString *)stringFromWaveType:(WaveTypeEnum)waveType;
+ (float)valueForWaveType:(WaveTypeEnum)waveType atPhase:(double)phase;

@end

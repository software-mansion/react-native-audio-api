#import "Constants.h"

@implementation Constants

+ (int)sampleRate {
    return 44100;
}

+ (double)mostPositiveSingleFloat {
    return FLT_MAX;
}

+ (double)nyquistFrequency {
    return [self sampleRate] / 2.0;
}

+ (double)maxDetune {
    return 1200 * log2([self mostPositiveSingleFloat]);
}

+ (double)maxGain {
    return [self mostPositiveSingleFloat];
}

+ (double)maxPan {
    return 1.0;
}

+ (double)maxFilterQ {
    return [self mostPositiveSingleFloat];
}

+ (double)maxFilterFrequency {
    return [self nyquistFrequency];
}

+ (double)minFilterFrequency {
    return 0.0;
}

+ (double)maxFilterGain {
    return 40 * log10([self mostPositiveSingleFloat]);
}

+ (double)minFilterGain {
    return -[self maxGain];
}

@end

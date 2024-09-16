#pragma once

#import <Foundation/Foundation.h>
#import <math.h>

@interface Constants : NSObject

+ (int)sampleRate;
+ (int)bufferSize;
+ (double)mostPositiveSingleFloat;
+ (double)nyquistFrequency;
+ (double)maxDetune;
+ (double)maxGain;
+ (double)maxPan;
+ (double)maxFilterQ;
+ (double)maxFilterFrequency;
+ (double)minFilterFrequency;
+ (double)maxFilterGain;
+ (double)minFilterGain;

@end

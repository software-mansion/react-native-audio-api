#pragma once

#import "PriorityQueue.h"
#import "ParamChange.h"

@class AudioContext;

@interface AudioParam : NSObject

@property (nonatomic, assign) float value;
@property (nonatomic, assign) float minValue;
@property (nonatomic, assign) float maxValue;
@property (nonatomic, assign) float defaultValue;
@property (nonatomic, strong) AudioContext *context;
@property (nonatomic, strong) PriorityQueue *timeline;

- (instancetype)initWithContext:(AudioContext *)context value:(float)value minValue:(float)minValue maxValue:(float)maxValue;

- (void)clean;

- (void)setValue:(float)value;

- (float)getValue;

- (float)getMinValue;

- (float)getMaxValue;

- (float)getDefaultValue;

- (double)getValueAtTime:(double)time;

- (double)getStartTime;

- (float)getStartValue;

- (void)setValueAtTime:(float)value time:(double)time;

- (double)calculateSetValueAtTime:(double)startTime endTime:(double)endTime startValue:(double)startValue endValue:(double)endValue currentTime:(double)currentTime;

- (void)linearRampToValueAtTime:(float)value time:(double)time;

- (double)calculateLinearValueWithStartTime:(double)startTime endTime:(double)endTime startValue:(double)startValue endValue:(double)endValue currentTime:(double)currentTime;

- (void)exponentialRampToValueAtTime:(float)value time:(double)time;

- (double)calculateExponentialRampToValueAtTime:(double)startTime endTime:(double)endTime startValue:(double)startValue endValue:(double)endValue currentTime:(double)currentTime;

@end

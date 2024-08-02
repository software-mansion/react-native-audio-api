#pragma once

@class AudioContext;

@interface AudioParam : NSObject

@property (nonatomic, assign) float value;
@property (nonatomic, assign) float minValue;
@property (nonatomic, assign) float maxValue;
@property (nonatomic, assign) float defaultValue;
@property (nonatomic, strong) AudioContext *context;

- (instancetype)init:(AudioContext *)context value:(float)value minValue:(float)minValue maxValue:(float)maxValue;

- (void)setValue:(float)value;

- (float)getValue;

- (float)getMinValue;

- (float)getMaxValue;

- (float)getDefaultValue;

@end

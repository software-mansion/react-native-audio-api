#pragma once

#import "AudioNode.h"
#import "AudioParam.h"
#import "FilterType.h"

@interface BiquadFilterNode : AudioNode

@property (nonatomic, strong) AudioParam *frequencyParam;
@property (nonatomic, strong) AudioParam *detuneParam;
@property (nonatomic, strong) AudioParam *QParam;
@property (nonatomic, strong) AudioParam *gainParam;

@property (nonatomic, assign) FilterTypeEnum filterType;

// delayed samples
@property (nonatomic, assign) float x1;
@property (nonatomic, assign) float x2;
@property (nonatomic, assign) float y1;
@property (nonatomic, assign) float y2;

// cofficients
@property (nonatomic, assign) float b0;
@property (nonatomic, assign) float b1;
@property (nonatomic, assign) float b2;
@property (nonatomic, assign) float a1;
@property (nonatomic, assign) float a2;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

- (void)setType:(NSString *)type;

- (NSString *)getType;

@end

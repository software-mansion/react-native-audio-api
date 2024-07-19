#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "WaveType.h"

@interface OscillatorNode : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) AVAudioPlayerNode *playerNode;
@property (nonatomic, strong) AVAudioPCMBuffer *buffer;
@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, assign) WaveTypeEnum waveType;
@property (nonatomic, assign) float frequency;
@property (nonatomic, assign) double sampleRate;

- (instancetype)initWithFrequency:(float)frequency;

- (void)start;

- (void)stop;

- (void)changeFrequency:(float)frequency;

- (float)getFrequency;

- (void)setType:(NSString *)type;

- (NSString *)getType;

@end

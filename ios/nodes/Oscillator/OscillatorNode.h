#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "WaveType.h"
#import "AudioScheduledSourceNode.h"

static const double OCTAVE_IN_CENTS = 12 * 100;

@interface OscillatorNode : AudioScheduledSourceNode

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) AVAudioPlayerNode *playerNode;
@property (nonatomic, strong) AVAudioPCMBuffer *buffer;
@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, assign) WaveTypeEnum waveType;
@property (nonatomic, assign) float frequency;
@property (nonatomic, assign) float detune;
@property (nonatomic, assign) double sampleRate;

- (instancetype)init;

- (void)start;

- (void)stop;

- (void)changeFrequency:(float)frequency;

- (float)getFrequency;

- (void)changeDetune:(float)detune;

- (float)getDetune;

- (void)setType:(NSString *)type;

- (NSString *)getType;

@end

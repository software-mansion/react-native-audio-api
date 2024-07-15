#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface IOSOscillator : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) AVAudioPlayerNode *playerNode;
@property (nonatomic, strong) AVAudioPCMBuffer *buffer;
@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, assign) float frequency;
@property (nonatomic, assign) double sampleRate;

- (instancetype)init:(float)frequency;

- (void)start;

- (void)stop;

@end

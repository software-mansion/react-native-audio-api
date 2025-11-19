#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

typedef void (^AudioReceiverBlock)(const AudioBufferList *inputBuffer, int numFrames);

@interface NativeAudioRecorder : NSObject

@property (nonatomic, assign) int bufferLength;
@property (nonatomic, assign) float sampleRate;

@property (nonatomic, strong) AVAudioSinkNode *sinkNode;
@property (nonatomic, copy) AVAudioSinkNodeReceiverBlock receiverSinkBlock;
@property (nonatomic, copy) AudioReceiverBlock receiverBlock;

@property (nonatomic, strong) AVAudioConverter *audioConverter;
@property (nonatomic, strong) AVAudioFormat *inputFormat;
@property (nonatomic, strong) AVAudioFormat *outputFormat;

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
                         bufferLength:(int)bufferLength
                           sampleRate:(float)sampleRate;

- (void)start;

- (void)stop;

- (void)cleanup;

@end

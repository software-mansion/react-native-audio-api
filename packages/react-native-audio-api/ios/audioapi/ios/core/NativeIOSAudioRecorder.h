#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

typedef void (^AudioReceiverBlock)(const AudioBufferList *inputBuffer, int numFrames, AVAudioTime *when);

@interface NativeIOSAudioRecorder : NSObject

@property (nonatomic, assign) int bufferLength;

@property (nonatomic, strong) AVAudioSinkNode *sinkNode;
@property (nonatomic, copy) AVAudioSinkNodeReceiverBlock receiverSinkBlock;
@property (nonatomic, copy) AudioReceiverBlock receiverBlock;

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock bufferLength:(int)bufferLength;

- (void)start;

- (void)stop;

- (void)cleanup;

@end

#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

typedef void (^AudioReceiverBlock)(AVAudioPCMBuffer *buffer, int numFrames, AVAudioTime *when);

@interface NativeIOSAudioRecorder : NSObject

@property (nonatomic, assign) bool isRunning;
@property (nonatomic, assign) int bufferLength;
@property (nonatomic, assign) bool enableVoiceProcessing;

@property (nonatomic, strong) NSString *tapId;
@property (nonatomic, strong) AVAudioNodeTapBlock tapBlock;
@property (nonatomic, copy) AudioReceiverBlock receiverBlock;

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
                         bufferLength:(int)bufferLength
                enableVoiceProcessing:(bool)enableVoiceProcessing;

- (void)start;

- (void)stop;

- (void)cleanup;

@end

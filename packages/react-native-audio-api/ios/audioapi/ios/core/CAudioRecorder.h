#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

typedef void (^AudioReceiverBlock)(AVAudioPCMBuffer *buffer, int numFrames, AVAudioTime *when);

@interface CAudioRecorder : NSObject

@property (nonatomic, assign) bool isRunning;
@property (nonatomic, assign) int bufferSize;
@property (nonatomic, strong) NSString *tapId;

@property (nonatomic, copy) AudioReceiverBlock receiverBlock;
@property (nonatomic, strong) AVAudioNodeTapBlock tapBlock;

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock;

- (void)start;

- (void)stop;

- (void)cleanup;

@end

#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <audioapi/ios/system/AudioEngine.h>

typedef void (^AudioReceiverBlock)(const AudioBufferList *inputBuffer, int numFrames);

@interface NativeAudioRecorder : NSObject

@property (nonatomic, copy) AVAudioSinkNodeReceiverBlock receiverSinkBlock;
@property (nonatomic, copy) AudioReceiverBlock receiverBlock;
@property (nonatomic, strong) AVAudioFormat *resolvedInputFormat;
@property (nonatomic, assign) int resolvedBufferSize;
@property (atomic, assign) BOOL inputArmed;
@property (nonatomic, assign) BOOL voiceProcessingEnabled;
@property (nonatomic, copy) void (^onInputNotification)(AudioEngineInputNotification);

- (instancetype)initWithReceiverBlock:(AudioReceiverBlock)receiverBlock
               voiceProcessingEnabled:(BOOL)voiceProcessingEnabled;

- (int)getBufferSize;
- (AVAudioFormat *)getResolvedInputFormat;
- (int)getResolvedBufferSize;
- (BOOL)refreshResolvedInputFormatReturningChanged:(BOOL *)formatChanged;
- (BOOL)start:(NSError **)error;

- (void)stop;

- (void)pause;

/// @return YES if the engine started.
- (BOOL)resume;

- (void)cleanup;

@end

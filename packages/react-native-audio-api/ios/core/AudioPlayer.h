#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

typedef void (^RenderAudioBlock)(AudioBufferList *outputBuffer, int numFrames);

@interface AudioPlayer : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, weak) AVAudioSession *audioSession;
@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, strong) AVAudioSourceNode *sourceNode;
@property (nonatomic, copy) RenderAudioBlock renderAudio;
@property (nonatomic, assign) int sampleRate;

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio;

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio sampleRate:(int)sampleRate;

- (int)getSampleRate;

- (void)start;

- (void)stop;

- (void)cleanup;

@end

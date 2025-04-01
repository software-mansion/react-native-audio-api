#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

typedef void (^RenderAudioBlock)(AudioBufferList *outputBuffer, int numFrames);

@interface AudioPlayer : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, weak) AVAudioSession *audioSession;
@property (nonatomic, weak) NSNotificationCenter *notificationCenter;
@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, strong) AVAudioSourceNode *sourceNode;
@property (nonatomic, copy) RenderAudioBlock renderAudio;
@property (nonatomic, assign) float sampleRate;
@property (nonatomic, assign) bool isRunning;
@property (nonatomic, strong) AVAudioSourceNodeRenderBlock renderBlock;
@property (nonatomic, assign) bool isAudioSessionActive;

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio;

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio sampleRate:(float)sampleRate;

- (float)getSampleRate;

- (void)start;

- (void)stop;

- (void)resume;

- (void)suspend;

- (void)cleanup;

- (void)setupAndInitAudioSession;

- (void)connectAudioEngine;

@end

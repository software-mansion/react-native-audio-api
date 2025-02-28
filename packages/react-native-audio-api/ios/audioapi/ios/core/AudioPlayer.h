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

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio;

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio sampleRate:(float)sampleRate;

- (float)getSampleRate;

- (void)start;

- (void)stop;

- (void)resume;

- (void)suspend;

- (void)cleanup;

- (void)setupAndInitAudioSession;

- (void)setupAndInitNotificationHandlers;

- (void)connectAudioEngine;

- (void)handleEngineConfigurationChange:(NSNotification *)notification;

@end

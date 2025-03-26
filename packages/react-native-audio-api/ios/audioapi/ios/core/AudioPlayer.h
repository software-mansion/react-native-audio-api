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
@property (nonatomic, assign) int channelCount;
@property (nonatomic, assign) bool isRunning;
@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, assign) bool configurationChanged;

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio channelCount:(int)channelCount;

- (instancetype)initWithRenderAudioBlock:(RenderAudioBlock)renderAudio
                              sampleRate:(float)sampleRate
                            channelCount:(int)channelCount;

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
- (void)handleInterruption:(NSNotification *)notification;

@end

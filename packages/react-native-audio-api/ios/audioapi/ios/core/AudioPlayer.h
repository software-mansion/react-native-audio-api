#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <IOSAudioManager.h>

typedef void (^RenderAudioBlock)(AudioBufferList *outputBuffer, int numFrames);

@interface AudioPlayer : NSObject

@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, strong) AVAudioSourceNode *sourceNode;
@property (nonatomic, weak) IOSAudioManager *audioManager;
@property (nonatomic, copy) RenderAudioBlock renderAudio;
@property (nonatomic, assign) float sampleRate;
@property (nonatomic, assign) int channelCount;
@property (nonatomic, assign) bool isRunning;
@property (nonatomic, strong) NSString *sourceNodeId;

- (instancetype)initWithAudioManager:(IOSAudioManager *)audioManager
                         renderAudio:(RenderAudioBlock)renderAudio
                        channelCount:(int)channelCount;

- (instancetype)initWithAudioManager:(IOSAudioManager *)audioManager
                         renderAudio:(RenderAudioBlock)renderAudio
                          sampleRate:(float)sampleRate
                        channelCount:(int)channelCount;

- (float)getSampleRate;

- (void)start;

- (void)stop;

- (void)resume;

- (void)suspend;

- (void)cleanup;

@end

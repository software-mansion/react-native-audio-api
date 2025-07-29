#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

typedef void (^RenderAudioBlock)(AudioBufferList *outputBuffer, int numFrames);

@interface NativeAudioPlayer : NSObject

@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, strong) AVAudioSourceNode *sourceNode;
@property (nonatomic, copy) RenderAudioBlock renderAudio;
@property (nonatomic, assign) float sampleRate;
@property (nonatomic, assign) int channelCount;
@property (nonatomic, strong) NSString *sourceNodeId;
@property (nonatomic, strong) AVAudioSourceNodeRenderBlock renderBlock;

- (instancetype)initWithRenderAudio:(RenderAudioBlock)renderAudio
                         sampleRate:(float)sampleRate
                       channelCount:(int)channelCount;

- (bool)start;

- (void)stop;

- (bool)resume;

- (void)suspend;

- (void)cleanup;

@end

#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface AudioEngine : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) NSMutableDictionary *sourceNodes;
@property (nonatomic, strong) NSMutableDictionary *sourceFormats;
@property (nonatomic, strong) NSString *tapId;
@property (nonatomic, strong) AVAudioMixerNode *inputMixer;

+ (instancetype)sharedInstance;
- (void)cleanup;
- (bool)rebuildAudioEngine;
- (void)startEngine;
- (void)stopEngine;
- (bool)isRunning;

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format;
- (void)detachSourceNodeWithId:(NSString *)sourceNodeId;

- (NSString *)installInputTap:(AVAudioNodeTapBlock)tapBlock
                 bufferLength:(int)bufferLength
        enableVoiceProcessing:(bool)enableVoiceProcessing;
- (void)removeInputTap:(NSString *)tapId;

- (void)startIfNecessary;
- (void)stopIfNecessary;

@end

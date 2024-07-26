#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "AudioContext.h"

@interface AudioNode : NSObject

@property (nonatomic, assign) NSInteger numberOfInputs;
@property (nonatomic, assign) NSInteger numberOfOutputs;
@property (nonatomic, strong) NSMutableArray<AudioNode *> *connectedNodes;
@property (nonatomic, strong) AudioContext *context;

- (instancetype)init:(AudioContext *)context;
- (void)process:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode;
- (void)connect:(AudioNode *)node;
- (void)disconnect:(AudioNode *)node;
- (void)syncPlayerNode:(AVAudioPlayerNode *)node;
- (void)clearPlayerNode:(AVAudioPlayerNode *)node;

@end

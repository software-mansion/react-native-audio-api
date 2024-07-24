#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface AudioNode : NSObject

@property (nonatomic, assign) NSInteger numberOfInputs;
@property (nonatomic, assign) NSInteger numberOfOutputs;
@property (nonatomic, strong) NSMutableArray<AudioNode *> *connectedNodes;

- (void)connect:(AudioNode *)node;
- (void)disconnect;
- (void)process:(AVAudioPCMBuffer *)buffer engine:(AVAudioEngine *)engine;

@end

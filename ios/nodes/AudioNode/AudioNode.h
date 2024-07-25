#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface AudioNode : NSObject

@property (nonatomic, assign) NSInteger numberOfInputs;
@property (nonatomic, assign) NSInteger numberOfOutputs;
@property (nonatomic, strong) NSMutableDictionary<NSUUID *, AudioNode *> *connectedNodes;
@property (nonatomic, strong) NSUUID *uuid;

- (void)connect:(AudioNode *)node;
- (void)disconnect:(AudioNode *)node;
- (void)disconnectAttachedNode:(AudioNode *)node;
- (void)process:(AVAudioPCMBuffer *)buffer engine:(AVAudioEngine *)engine uuid:(NSUUID *)uuid;

@end

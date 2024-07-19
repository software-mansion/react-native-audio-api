#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface AudioNode : NSObject

@property (nonatomic, readonly) NSInteger numberOfInputs;
@property (nonatomic, readonly) NSInteger numberOfOutputs;
@property (nonatomic, strong) NSMutableArray<AudioNode *> *connectedNodes;

- (void)connect:(AudioNode *)node;
- (void)disconnect;
- (void)process;

@end

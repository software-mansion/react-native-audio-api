#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface IOSOscillator : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) AVAudioPlayerNode *playerNode;

- (instancetype)init;

- (void)start;

- (void)stop;

@end

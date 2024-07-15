#import <IOSOscillator.h>

#define SAMPLE_RATE 44100
#define DURATION 2.0

@implementation IOSOscillator {
    
}

- (instancetype)init {
    if (self = [super init]) {

    }

    return self;
}

- (AVAudioPCMBuffer *)generateSineWaveBufferWithFormat:(AVAudioFormat *)audioFormat frequency:(float)frequency duration:(float)duration {
    AVAudioFrameCount frameCount = (AVAudioFrameCount)(duration * SAMPLE_RATE);
    AVAudioPCMBuffer *pcmBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:audioFormat frameCapacity:frameCount];
    pcmBuffer.frameLength = frameCount;

    float *data = pcmBuffer.floatChannelData[0];
    float theta = 0.0;
    float theta_increment = 2.0 * M_PI * frequency / SAMPLE_RATE;
    for (AVAudioFrameCount frame = 0; frame < frameCount; frame++) {
        data[frame] = sin(theta);
        theta += theta_increment;
        if (theta > 2.0 * M_PI) {
            theta -= 2.0 * M_PI;
        }
    }

    return pcmBuffer;
}

- (void)start {
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.playerNode = [[AVAudioPlayerNode alloc] init];
    [self.audioEngine attachNode:self.playerNode];

    AVAudioFormat *audioFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                                                                    sampleRate:SAMPLE_RATE
                                                                                                      channels:1
                                                                                                  interleaved:NO];

    [self.audioEngine connect:self.playerNode to:self.audioEngine.mainMixerNode format:audioFormat];
    AVAudioPCMBuffer *pcmBuffer = [self generateSineWaveBufferWithFormat:audioFormat frequency: 440 duration:DURATION];

    [self.playerNode scheduleBuffer:pcmBuffer atTime:nil options:AVAudioPlayerNodeBufferLoops completionHandler:nil];

		NSError *error = nil;
    if (![self.audioEngine startAndReturnError:&error]) {
        NSLog(@"Error starting audio engine: %@", error);
        return;
    }

    [self.playerNode play];
}

- (void)stop {
    [self.playerNode stop];
}

@end

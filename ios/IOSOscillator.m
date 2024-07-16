#import <IOSOscillator.h>

@implementation IOSOscillator {}

- (instancetype)init:(float)frequency {
    if (self = [super init]) {
        self.frequency = frequency;
        self.sampleRate = 44100;

        self.audioEngine = [[AVAudioEngine alloc] init];
        self.playerNode = [[AVAudioPlayerNode alloc] init];

        self.format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.sampleRate channels:1];
        AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)self.sampleRate;
        self.buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:self.format frameCapacity:bufferFrameCapacity];
        self.buffer.frameLength = bufferFrameCapacity;

        [self.audioEngine attachNode:self.playerNode];
        [self.audioEngine connect:self.playerNode to:self.audioEngine.mainMixerNode format: self.format];
        [self setBuffer];

        NSError *error = nil;
        if (![self.audioEngine startAndReturnError:&error]) {
            NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
        }
    }

    return self;
}

- (void)start {
    [self.playerNode scheduleBuffer:self.buffer atTime:nil options:AVAudioPlayerNodeBufferLoops completionHandler:nil];
    [self.playerNode play];
}

- (void)stop {
    [self.playerNode stop];
}

- (void)setBuffer {
    AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)self.sampleRate;

    double thetaIncrement = 2.0 * M_PI * self.frequency / self.sampleRate;
    double theta = 0.0;
    float *audioBufferPointer = self.buffer.floatChannelData[0];

    for (int frame = 0; frame < bufferFrameCapacity; frame++) {
        audioBufferPointer[frame] = sin(theta);
        theta += thetaIncrement;
        if (theta > 2.0 * M_PI) {
            theta -= 2.0 * M_PI;
        }
    }
}

- (void)changeFrequency:(float)frequency {
    self.frequency = frequency;
    [self setBuffer];
}

@end

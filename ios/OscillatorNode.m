#import <OscillatorNode.h>

static const float FULL_CIRCLE_RADIANS = 2 * M_PI;

@implementation OscillatorNode {}

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

    double phaseIncrement = FULL_CIRCLE_RADIANS * self.frequency / self.sampleRate;
    double phase = 0.0;
    float *audioBufferPointer = self.buffer.floatChannelData[0];

    for (int frame = 0; frame < bufferFrameCapacity; frame++) {
        audioBufferPointer[frame] = sin(phase);
        phase += phaseIncrement;
        if (phase > FULL_CIRCLE_RADIANS) {
            phase -= FULL_CIRCLE_RADIANS;
        }
    }
}

- (void)changeFrequency:(float)frequency {
    self.frequency = frequency;
    [self setBuffer];
}

@end

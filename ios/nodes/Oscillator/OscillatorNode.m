#import <OscillatorNode.h>

@implementation OscillatorNode {}

- (instancetype)init {
    if (self = [super init]) {
        self.frequency = 440;
        self.sampleRate = 44100;
        self.waveType = WaveTypeSine;

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
        audioBufferPointer[frame] = [WaveType valueForWaveType:self.waveType atPhase:phase];

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

- (float)getFrequency {
    return self.frequency;
}

- (void)setType:(NSString *)type {
    self.waveType = [WaveType waveTypeFromString:type];
    [self setBuffer]; // Update the buffer with the new wave type
}

- (NSString *)getType {
    return [WaveType stringFromWaveType:self.waveType];
}

@end

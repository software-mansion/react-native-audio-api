#import <OscillatorNode.h>

@implementation OscillatorNode

- (instancetype)init {
    if (self != [super init]) {
      return self;
    }

    self.frequency = 440;
    self.detune = 0;
    self.sampleRate = 44100;
    self.waveType = WaveTypeSine;
    self.numberOfOutputs = 1;

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

    return self;
}

- (void)start {
    [self process:self.buffer engine:self.audioEngine];
    [self.playerNode scheduleBuffer:self.buffer atTime:nil options:AVAudioPlayerNodeBufferLoops completionHandler:nil];
    [self.playerNode play];
}

- (void)stop {
    [self.playerNode stop];
}

- (void)setBuffer {
    AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)self.sampleRate;
    
    // Convert cents to HZ
    double detuneRatio = pow(2.0, self.detune / OCTAVE_IN_CENTS);
    double detunedFrequency = detuneRatio * self.frequency;
    double phaseIncrement = FULL_CIRCLE_RADIANS * detunedFrequency / self.sampleRate;
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

- (void)changeDetune:(float)detune {
    self.detune = detune;
    [self setBuffer];
}

- (float)getDetune {
    return self.detune;
}

- (void)setType:(NSString *)type {
    self.waveType = [WaveType waveTypeFromString:type];
    [self setBuffer]; // Update the buffer with the new wave type
}

- (NSString *)getType {
    return [WaveType stringFromWaveType:self.waveType];
}

@end

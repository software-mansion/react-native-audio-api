#import <OscillatorNode.h>

@implementation OscillatorNode

- (instancetype)init:(AudioContext *)context {
    if (self != [super init:context]) {
      return self;
    }
    
    _frequencyParam = [[AudioParam alloc] init:context value:440 minValue:0 maxValue:1600];
    _detuneParam = [[AudioParam alloc] init:context value:0 minValue:-100 maxValue:100];
    _waveType = WaveTypeSine;
    self.numberOfOutputs = 1;
    self.numberOfInputs = 0;

    _playerNode = [[AVAudioPlayerNode alloc] init];
    _playerNode.volume = 0.5;

    _format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:self.context.sampleRate channels:1];
    AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)self.context.sampleRate;
    _buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:self.format frameCapacity:bufferFrameCapacity];
    _buffer.frameLength = bufferFrameCapacity;

    [self.context.audioEngine attachNode:self.playerNode];
    [self.context.audioEngine connect:self.playerNode to:self.context.audioEngine.mainMixerNode format: self.format];
    [self setBuffer];

    if (!self.context.audioEngine.isRunning) {
        NSError *error = nil;
        if (![self.context.audioEngine startAndReturnError:&error]) {
            NSLog(@"Error starting audio engine: %@", [error localizedDescription]);
        }
    }

    [self.context connectOscillator:self];

    return self;
}

- (void)start {
    [self process:_buffer playerNode:_playerNode];
    [self.playerNode scheduleBuffer:_buffer atTime:nil options:AVAudioPlayerNodeBufferLoops completionHandler:nil];
    [self.playerNode play];
}

- (void)stop {
    [self.playerNode stop];
}

- (void)process:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode {
    [self setBuffer];
    [super process:buffer playerNode:playerNode];
}

- (void)setBuffer {
    AVAudioFrameCount bufferFrameCapacity = (AVAudioFrameCount)self.context.sampleRate;
    
    // Convert cents to HZ
    double detuneRatio = pow(2.0, [_detuneParam getValue] / OCTAVE_IN_CENTS);
    double detunedFrequency = round(detuneRatio * [_frequencyParam getValue]);
    double phaseIncrement = FULL_CIRCLE_RADIANS * detunedFrequency / self.context.sampleRate;
    double phase = 0.0;
    float *audioBufferPointer = _buffer.floatChannelData[0];

    for (int frame = 0; frame < bufferFrameCapacity; frame++) {
        audioBufferPointer[frame] = [WaveType valueForWaveType:_waveType atPhase:phase];

        phase += phaseIncrement;
        if (phase > FULL_CIRCLE_RADIANS) {
            phase -= FULL_CIRCLE_RADIANS;
        }
    }
}

- (void)setType:(NSString *)type {
    _waveType = [WaveType waveTypeFromString:type];
    [self setBuffer]; // Update the buffer with the new wave type
}

- (NSString *)getType {
    return [WaveType stringFromWaveType:self.waveType];
}

@end
